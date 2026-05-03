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

namespace fs = std::filesystem;

struct Job {
    std::string name;
    std::string run;
    std::vector<std::string> depends;
};

// Parsear el pipeline.yml
static std::vector<Job> parsePipeline() {
    fs::path pipelinePath = fs::path(".mank") / "ci" / "pipeline.yml";
    if (!fs::exists(pipelinePath)) {
        //Log::error("No pipeline found at .mank/ci/pipeline.yml");
        return {};
    }

    YAML::Node config = YAML::LoadFile(pipelinePath.string());
    std::vector<Job> jobs;

    if (!config["jobs"]) return jobs;

    for (const auto& entry : config["jobs"]) {
        Job job;
        job.name = entry.first.as<std::string>();
        job.run  = entry.second["run"].as<std::string>();

        if (entry.second["depends"])
            for (const auto& dep : entry.second["depends"])
                job.depends.push_back(dep.as<std::string>());

        jobs.push_back(job);
    }
    return jobs;
}

// Obtener el siguiente número de run para un job
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

// En launchJob, antes de lanzar verificar el trigger
static bool matchesTrigger(const Job& job, const std::string& trigger) {
    // Si no se especifica trigger, correr siempre
    if (trigger.empty()) return true;
    // Leer el on: del pipeline y verificar
    fs::path pipelinePath = fs::path(".mank") / "ci" / "pipeline.yml";
    YAML::Node config = YAML::LoadFile(pipelinePath.string());
    if (!config["on"]) return true;
    for (const auto& t : config["on"])
        if (t.as<std::string>() == trigger) return true;
    return false;
}

// Lanzar un job en segundo plano
static void launchJob(const Job& job) {
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
        // Proceso hijo — redirigir stdout y stderr al log
        int fd = open(logFile.string().c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) _exit(1);

        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);

        // Escribir header en el log
        std::time_t now = std::time(nullptr);
        std::string header = "=== Job: " + job.name + " ===\n"
                           + "Run: " + std::to_string(runNumber) + "\n"
                           + "Date: " + std::string(std::ctime(&now))
                           + "Command: " + job.run + "\n"
                           + "---\n";
        write(STDOUT_FILENO, header.c_str(), header.size());

        // Ejecutar el comando
        int ret = system(job.run.c_str());

        // Escribir resultado
        std::string footer = "\n--- Exit code: " + std::to_string(WEXITSTATUS(ret)) + " ---\n";
        write(STDOUT_FILENO, footer.c_str(), footer.size());

        // Notificación del sistema
        std::string status  = WEXITSTATUS(ret) == 0 ? "✓ passed" : "✗ failed";
        std::string notifCmd = "notify-send \"mank ci\" \"" + job.name + " " + status + "\"";
        system(notifCmd.c_str());

        _exit(WEXITSTATUS(ret));
    }

    // Proceso padre — no esperar, el job corre en segundo plano
    // Actualizar symlink latest.log
    if (fs::exists(latest))
        fs::remove(latest);
    fs::create_symlink(logFile.filename(), latest);

    Log::log("Job launched in background: " + job.name + " (run #" + std::to_string(runNumber) + ")");
}

// Ordenar jobs respetando dependencias
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

// Mostrar logs
static int showLogs(const std::string& jobName, bool all) {
    fs::path runsDir = fs::path(".mank") / "ci" / "runs";

    if (!fs::exists(runsDir)) {
        Log::log("No runs yet.");
        return 0;
    }

    // Sin job específico — mostrar latest de todos
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
        // Solo el último run
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
        // Todos los runs ordenados
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

int Mank::ci(const std::vector<std::string>& args, const std::string& trigger) {
    if (args.empty()) {
        Log::error("Usage: mank ci <run|logs> [job] [--all]");
        return 1;
    }

    std::string subcommand = args[0];

    // ── mank ci logs ──────────────────────────────────────────
    if (subcommand == "logs") {
        std::string jobName;
        bool all = false;

        if (args.size() >= 2 && args[1] != "--all")
            jobName = args[1];
        if (args.back() == "--all")
            all = true;

        return showLogs(jobName, all);
    }

    // ── mank ci run ───────────────────────────────────────────
    if (subcommand == "run") {
        auto jobs = parsePipeline();
        if (jobs.empty()) return 1;

        if (args.size() >= 2) {
            std::string target = args[1];
            for (const auto& job : jobs) {
                if (job.name == target) {
                    launchJob(job);
                    return 0;
                }
            }
            Log::error("Job not found: " + target);
            return 1;
        }

        auto sorted = sortJobs(jobs);
        for (const auto& job : sorted) {
            // Filtrar por trigger solo si se especificó uno
            if (!trigger.empty() && !matchesTrigger(job, trigger)) continue;
            launchJob(job);
        }

        return 0;
    }

    Log::error("Unknown ci subcommand: " + subcommand);
    Log::info("Usage: mank ci <run|logs> [job] [--all]");
    return 1;
}