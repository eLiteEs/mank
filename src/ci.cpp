/*
 * Version tracking program
 * Copyright (C) 2026  mank
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "mank.hpp"
#include "objects.hpp"
#include "log.hpp"
#include "decorations.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <yaml-cpp/yaml.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctime>
#include <set>
#include <functional>
#include <algorithm>

namespace fs = std::filesystem;

// ── Estructuras ──────────────────────────────────
struct Job {
    std::string name;
    std::string run;
    std::vector<std::string> steps;
    std::vector<std::string> depends;
    std::string condition;                    // NUEVO: "platform == 'linux' && branch == 'main'"
    std::map<std::string, std::string> env;   // NUEVO: variables de entorno del job
    bool isPrivate = false;
};

struct Pipeline {
    std::string name;
    std::vector<std::string> triggers;        // NUEVO: ["commit", "merge", "manual"]
    std::map<std::string, std::string> env;   // NUEVO: variables del pipeline
    std::vector<Job> jobs;
};

// ── Variables predefinidas de mank ──────────────
static std::map<std::string, std::string> getMankEnv() {
    std::map<std::string, std::string> env;
    
    // Plataforma
    #ifdef __linux__
        env["MANK_PLATFORM"] = "linux";
    #elif __APPLE__
        env["MANK_PLATFORM"] = "macos";
    #elif _WIN32
        env["MANK_PLATFORM"] = "windows";
    #else
        env["MANK_PLATFORM"] = "unknown";
    #endif
    
    // Arquitectura
    #ifdef __x86_64__
        env["MANK_ARCH"] = "x86_64";
    #elif __aarch64__
        env["MANK_ARCH"] = "aarch64";
    #else
        env["MANK_ARCH"] = "unknown";
    #endif
    
    // Info del repo
    env["MANK_BRANCH"] = Objects::getCurrentBranch();
    env["MANK_REPO"] = Objects::getConfig("core", "name");
    
    // Último commit
    std::ifstream refFile(Objects::getCurrentRef());
    if (refFile.is_open()) {
        std::string hash;
        std::getline(refFile, hash);
        env["MANK_COMMIT"] = hash;
        env["MANK_COMMIT_SHORT"] = hash.substr(0, 8);
    }
    
    // Último tag (si existe)
    auto tags = Objects::listTags();
    if (!tags.empty()) {
        env["MANK_TAG"] = tags.back().first;
        env["MANK_TAG_HASH"] = tags.back().second;
    }
    
    // Fecha/hora
    std::time_t now = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", std::localtime(&now));
    env["MANK_DATE"] = buf;
    std::strftime(buf, sizeof(buf), "%H:%M:%S", std::localtime(&now));
    env["MANK_TIME"] = buf;
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
    env["MANK_DATETIME"] = buf;
    
    return env;
}

// ── Evaluar condiciones ─────────────────────────
static bool evaluateCondition(const std::string& condition, 
                              const std::map<std::string, std::string>& env) {
    if (condition.empty()) return true;
    
    // Parsear condición simple: "variable == 'valor'"
    auto cond = condition;
    
    // Eliminar espacios extra
    cond.erase(0, cond.find_first_not_of(" \t"));
    cond.erase(cond.find_last_not_of(" \t") + 1);
    
    // Detectar operadores
    std::string op;
    size_t opPos = std::string::npos;
    
    if ((opPos = cond.find("==")) != std::string::npos) op = "==";
    else if ((opPos = cond.find("!=")) != std::string::npos) op = "!=";
    else if ((opPos = cond.find("&&")) != std::string::npos) op = "&&";
    else if ((opPos = cond.find("||")) != std::string::npos) op = "||";
    
    if (opPos == std::string::npos) {
        // Puede ser una variable booleana (ej: "tag != ''")
        return true;
    }
    
    std::string left = cond.substr(0, opPos);
    std::string right = cond.substr(opPos + op.length());
    
    // Trim
    left.erase(0, left.find_first_not_of(" \t"));
    left.erase(left.find_last_not_of(" \t") + 1);
    right.erase(0, right.find_first_not_of(" \t"));
    right.erase(right.find_last_not_of(" \t") + 1);
    
    // Resolver variables
    auto resolveVar = [&](const std::string& var) -> std::string {
        // Quitar comillas
        if ((var.front() == '\'' && var.back() == '\'') ||
            (var.front() == '"' && var.back() == '"'))
            return var.substr(1, var.length() - 2);
        
        // Buscar en env
        if (env.count(var))
            return env.at(var);
        
        return var;  // literal
    };
    
    std::string leftVal = resolveVar(left);
    std::string rightVal = resolveVar(right);
    
    if (op == "==") return leftVal == rightVal;
    if (op == "!=") return leftVal != rightVal;
    
    // Operadores lógicos (recursivo simple)
    if (op == "&&") {
        return evaluateCondition(left, env) && evaluateCondition(right, env);
    }
    if (op == "||") {
        return evaluateCondition(left, env) || evaluateCondition(right, env);
    }
    
    return true;
}

// ── Parsear pipelines (múltiples archivos) ─────
static std::vector<Pipeline> parseAllPipelines() {
    std::vector<Pipeline> pipelines;
    fs::path ciDir = fs::path(".mank") / "ci";
    
    if (!fs::exists(ciDir)) return pipelines;
    
    // Cargar pipelines públicos
    for (const auto& entry : fs::directory_iterator(ciDir)) {
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".yml" && 
            entry.path().extension() != ".yaml") continue;
        if (entry.path().filename() == "private") continue;  // saltar dir
        
        YAML::Node config;
        try {
            config = YAML::LoadFile(entry.path().string());
        } catch (...) {
            continue;
        }
        
        if (!config["jobs"]) continue;
        
        Pipeline pipe;
        pipe.name = config["pipeline"] 
            ? config["pipeline"].as<std::string>() 
            : entry.path().stem().string();
        
        // Triggers
        if (config["on"]) {
            for (const auto& t : config["on"])
                pipe.triggers.push_back(t.as<std::string>());
        }
        
        // Variables del pipeline
        if (config["env"]) {
            for (const auto& envVar : config["env"]) {
                pipe.env[envVar.first.as<std::string>()] = 
                    envVar.second.as<std::string>();
            }
        }
        
        // Jobs
        for (const auto& jobEntry : config["jobs"]) {
            Job job;
            job.name = jobEntry.first.as<std::string>();
            
            if (jobEntry.second["if"])
                job.condition = jobEntry.second["if"].as<std::string>();
            
            if (jobEntry.second["run"]) {
                job.run = jobEntry.second["run"].as<std::string>();
            } else if (jobEntry.second["steps"]) {
                for (const auto& step : jobEntry.second["steps"])
                    if (step["run"])
                        job.steps.push_back(step["run"].as<std::string>());
            }
            
            if (jobEntry.second["depends"])
                for (const auto& dep : jobEntry.second["depends"])
                    job.depends.push_back(dep.as<std::string>());
            
            if (jobEntry.second["private"])
                job.isPrivate = jobEntry.second["private"].as<bool>();
            
            // Variables del job
            if (jobEntry.second["env"]) {
                for (const auto& envVar : jobEntry.second["env"]) {
                    job.env[envVar.first.as<std::string>()] = 
                        envVar.second.as<std::string>();
                }
            }
            
            pipe.jobs.push_back(job);
        }
        
        pipelines.push_back(pipe);
    }
    
    // Cargar pipelines privados
    fs::path privateDir = ciDir / "private";
    if (fs::exists(privateDir)) {
        for (const auto& entry : fs::directory_iterator(privateDir)) {
            if (!entry.is_regular_file()) continue;
            if (entry.path().extension() != ".yml" && 
                entry.path().extension() != ".yaml") continue;
            
            YAML::Node config;
            try {
                config = YAML::LoadFile(entry.path().string());
            } catch (...) {
                continue;
            }
            
            if (!config["jobs"]) continue;
            
            Pipeline pipe;
            pipe.name = config["pipeline"] 
                ? config["pipeline"].as<std::string>() 
                : entry.path().stem().string();
            
            if (config["on"]) {
                for (const auto& t : config["on"])
                    pipe.triggers.push_back(t.as<std::string>());
            }
            
            if (config["env"]) {
                for (const auto& envVar : config["env"]) {
                    pipe.env[envVar.first.as<std::string>()] = 
                        envVar.second.as<std::string>();
                }
            }
            
            for (const auto& jobEntry : config["jobs"]) {
                Job job;
                job.name = jobEntry.first.as<std::string>();
                job.isPrivate = true;
                
                if (jobEntry.second["if"])
                    job.condition = jobEntry.second["if"].as<std::string>();
                
                if (jobEntry.second["run"]) {
                    job.run = jobEntry.second["run"].as<std::string>();
                } else if (jobEntry.second["steps"]) {
                    for (const auto& step : jobEntry.second["steps"])
                        if (step["run"])
                            job.steps.push_back(step["run"].as<std::string>());
                }
                
                if (jobEntry.second["depends"])
                    for (const auto& dep : jobEntry.second["depends"])
                        job.depends.push_back(dep.as<std::string>());
                
                if (jobEntry.second["env"]) {
                    for (const auto& envVar : jobEntry.second["env"]) {
                        job.env[envVar.first.as<std::string>()] = 
                            envVar.second.as<std::string>();
                    }
                }
                
                pipe.jobs.push_back(job);
            }
            
            pipelines.push_back(pipe);
        }
    }
    
    return pipelines;
}

// ── Filtrar pipelines por trigger ───────────────
static std::vector<Pipeline> filterPipelines(
    const std::vector<Pipeline>& pipelines, 
    const std::string& trigger) {
    
    std::vector<Pipeline> filtered;
    for (const auto& pipe : pipelines) {
        // Si no se especifica trigger, solo manuales
        if (trigger.empty()) {
            if (std::find(pipe.triggers.begin(), pipe.triggers.end(), "manual") 
                != pipe.triggers.end())
                filtered.push_back(pipe);
            continue;
        }
        
        // Si coincide el trigger
        if (std::find(pipe.triggers.begin(), pipe.triggers.end(), trigger) 
            != pipe.triggers.end())
            filtered.push_back(pipe);
    }
    return filtered;
}

// ── Preparar entorno para un job ───────────────
static void setupJobEnv(const Pipeline& pipe, const Job& job) {
    // 1. Variables de mank
    auto mankEnv = getMankEnv();
    for (const auto& [key, val] : mankEnv) {
        setenv(key.c_str(), val.c_str(), 1);
    }
    
    // 2. Variables del pipeline
    for (const auto& [key, val] : pipe.env) {
        // Reemplazar variables en el valor
        std::string resolved = val;
        for (const auto& [mankKey, mankVal] : mankEnv) {
            std::string placeholder = "$" + mankKey;
            size_t pos = resolved.find(placeholder);
            if (pos != std::string::npos)
                resolved.replace(pos, placeholder.length(), mankVal);
        }
        setenv(key.c_str(), resolved.c_str(), 1);
    }
    
    // 3. Variables del job (sobreescriben)
    for (const auto& [key, val] : job.env) {
        std::string resolved = val;
        for (const auto& [mankKey, mankVal] : mankEnv) {
            std::string placeholder = "$" + mankKey;
            size_t pos = resolved.find(placeholder);
            if (pos != std::string::npos)
                resolved.replace(pos, placeholder.length(), mankVal);
        }
        setenv(key.c_str(), resolved.c_str(), 1);
    }
}

static int getNextRunNumber(const std::string& jobName) {
    fs::path runDir = fs::path(".mank") / "ci" / "runs" / jobName;
    fs::create_directories(runDir);

    int max = 0;
    for (const auto& entry : fs::directory_iterator(runDir)) {
        std::string fname = entry.path().filename().string();
        if (fname == "latest.log") continue;
        try {
            int n = std::stoi(entry.path().stem().string());
            if (n > max) max = n;
        } catch (...) {}
    }
    return max + 1;
}

// Mostrar logs (actualizado para pipelines)
static int showLogs(const std::string& jobName, bool all) {
    fs::path runsDir = fs::path(".mank") / "ci" / "runs";

    if (!fs::exists(runsDir)) {
        Log::log("No runs yet.");
        return 0;
    }

    if (jobName.empty()) {
        for (const auto& entry : fs::directory_iterator(runsDir)) {
            if (!entry.is_directory()) continue;
            fs::path latest = entry.path() / "latest.log";
            if (!fs::exists(latest)) continue;

            std::cout << ansi::BOLD << ansi::FG_CYAN
                      << "=== " << entry.path().filename().string() << " (latest) ==="
                      << ansi::RESET << "\n";

            std::ifstream f(latest);
            std::cout << f.rdbuf() << "\n";
        }
        return 0;
    }

    fs::path jobDir = runsDir / jobName;
    if (!fs::exists(jobDir)) {
        Log::error("No runs found for job: " + jobName);
        return 1;
    }

    if (!all) {
        fs::path latest = jobDir / "latest.log";
        if (!fs::exists(latest)) {
            Log::error("No runs yet for job: " + jobName);
            return 1;
        }
        std::cout << ansi::BOLD << ansi::FG_CYAN
                  << "=== " << jobName << " (latest) ==="
                  << ansi::RESET << "\n";
        std::ifstream f(latest);
        std::cout << f.rdbuf() << "\n";
    } else {
        std::vector<fs::path> logs;
        for (const auto& entry : fs::directory_iterator(jobDir)) {
            if (entry.path().filename() == "latest.log") continue;
            logs.push_back(entry.path());
        }
        std::sort(logs.begin(), logs.end());

        for (const auto& logPath : logs) {
            std::cout << ansi::BOLD << ansi::FG_CYAN
                      << "=== " << jobName << " - " << logPath.filename().string() << " ==="
                      << ansi::RESET << "\n";
            std::ifstream f(logPath);
            std::cout << f.rdbuf() << "\n";
        }
    }
    return 0;
}

// ── Lanzar un job (actualizado) ────────────────
static void launchJob(const Pipeline& pipe, const Job& job) {
    auto mankEnv = getMankEnv();
    
    // Evaluar condición
    if (!job.condition.empty()) {
        // Combinar entornos para la evaluación
        std::map<std::string, std::string> evalEnv = mankEnv;
        for (const auto& [key, val] : pipe.env) evalEnv[key] = val;
        for (const auto& [key, val] : job.env) evalEnv[key] = val;
        
        if (!evaluateCondition(job.condition, evalEnv)) {
            Log::log("Skipping job '" + job.name + "' (condition not met)");
            return;
        }
    }
    
    int runNumber = getNextRunNumber(job.name);
    fs::path runDir  = fs::path(".mank") / "ci" / "runs" / job.name;
    fs::path logFile = runDir / (std::to_string(runNumber) + ".log");
    fs::path latest  = runDir / "latest.log";

    pid_t pid = fork();
    if (pid < 0) {
        Log::error("Failed to fork process for job: " + job.name);
        return;
    }

    if (pid == 0) {
        // Configurar entorno
        setupJobEnv(pipe, job);
        
        int fd = open(logFile.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) _exit(1);

        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        std::time_t now = std::time(nullptr);
        std::string header = "=== Pipeline: " + pipe.name + " ===\n"
                   + "Job: " + job.name + "\n"
                   + "Run: " + std::to_string(runNumber) + "\n"
                   + "Date: " + std::string(std::ctime(&now))
                   + "Branch: " + mankEnv["MANK_BRANCH"] + "\n"
                   + "Commit: " + mankEnv["MANK_COMMIT_SHORT"] + "\n";
        
        if (!job.condition.empty())
            header += "Condition: " + job.condition + "\n";
        
        header += "---\n";
        write(STDOUT_FILENO, header.c_str(), header.size());

        int exitCode = 0;

        if (!job.steps.empty()) {
            for (const auto& step : job.steps) {
                std::string stepHeader = "\n$ " + step + "\n";
                write(STDOUT_FILENO, stepHeader.c_str(), stepHeader.size());

                int ret = system(step.c_str());
                exitCode = WEXITSTATUS(ret);

                if (exitCode != 0) {
                    std::string msg = "\n--- Step failed: " + step + 
                                     " (exit " + std::to_string(exitCode) + ") ---\n";
                    write(STDOUT_FILENO, msg.c_str(), msg.size());
                    break;
                }
            }
        } else {
            std::string stepHeader = "\n$ " + job.run + "\n";
            write(STDOUT_FILENO, stepHeader.c_str(), stepHeader.size());
            int ret = system(job.run.c_str());
            exitCode = WEXITSTATUS(ret);
        }

        std::string footer = "\n--- Exit code: " + std::to_string(exitCode) + " ---\n";
        write(STDOUT_FILENO, footer.c_str(), footer.size());

        std::string status = exitCode == 0 ? "✓ passed" : "✗ failed";
        std::string notifCmd = "notify-send \"mank ci\" \"" + job.name + 
                               " " + status + "\"";
        system(notifCmd.c_str());

        _exit(exitCode);
    }

    if (fs::exists(latest))
        fs::remove(latest);
    fs::create_symlink(logFile.filename(), latest);

    Log::log("Job launched: " + job.name + 
             (job.isPrivate ? " [private]" : "") + 
             " (run #" + std::to_string(runNumber) + ")");
}

// ── Ordenar jobs (ahora por pipeline) ──────────
static std::vector<Job> sortJobs(const std::vector<Job>& jobs) {
    std::vector<Job> sorted;
    std::set<std::string> done;

    auto findJob = [&](const std::string& name) -> const Job* {
        for (const auto& j : jobs)
            if (j.name == name) return &j;
        return nullptr;
    };

    std::function<void(const Job&)> resolve = [&](const Job& job) {
        if (done.count(job.name)) return;
        for (const auto& dep : job.depends) {
            const Job* depJob = findJob(dep);
            if (depJob) resolve(*depJob);
        }
        sorted.push_back(job);
        done.insert(job.name);
    };

    for (const auto& job : jobs)
        resolve(job);

    return sorted;
}

// ── Ejecutar pipelines ──────────────────────────
int Mank::ci(const std::vector<std::string>& args, const std::string& trigger) {
    if (args.empty()) {
        Log::error("Usage: mank ci <run|logs> [job] [--all]");
        return 1;
    }

    std::string subcommand = args[0];

    if (subcommand == "logs") {
        std::string jobName;
        bool all = false;
        if (args.size() >= 2 && args[1] != "--all")
            jobName = args[1];
        if (args.back() == "--all")
            all = true;
        return showLogs(jobName, all);
    }

    if (subcommand == "run") {
        auto allPipelines = parseAllPipelines();
        if (allPipelines.empty()) {
            Log::log("No pipelines found.");
            return 1;
        }

        auto pipelines = filterPipelines(allPipelines, trigger);
        if (pipelines.empty()) {
            Log::log("No pipelines for trigger: " + 
                     (trigger.empty() ? "manual" : trigger));
            return 0;
        }

        // Si se especifica job, buscar en todos los pipelines
        if (args.size() >= 2) {
            std::string target = args[1];
            bool found = false;
            for (const auto& pipe : pipelines) {
                for (const auto& job : pipe.jobs) {
                    if (job.name == target) {
                        launchJob(pipe, job);
                        found = true;
                    }
                }
            }
            if (!found) {
                Log::error("Job not found: " + target);
                return 1;
            }
            return 0;
        }

        // Ejecutar todos los pipelines
        for (const auto& pipe : pipelines) {
            std::cout << ansi::BOLD << pipe.name << ansi::RESET << std::endl;            
            
	    auto sorted = sortJobs(pipe.jobs);
            for (const auto& job : sorted) {
                launchJob(pipe, job);
            }
        }

        return 0;
    }

    Log::error("Unknown ci subcommand: " + subcommand);
    return 1;
}
