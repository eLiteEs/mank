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
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

// Obtener el HEAD commit de un repo de mank
static std::string getRepoHead(const fs::path& repoPath) {
    // Leer HEAD para obtener la rama
    std::ifstream head(repoPath / ".mank" / "HEAD");
    if (!head.is_open()) return "";

    std::string line;
    std::getline(head, line);

    const std::string prefix = "ref: refs/heads/";
    if (line.substr(0, prefix.size()) != prefix) return "";
    std::string branch = line.substr(prefix.size());

    // Leer el commit de la rama
    std::ifstream ref(repoPath / ".mank" / "refs" / "heads" / branch);
    if (!ref.is_open()) return "";

    std::string commit;
    std::getline(ref, commit);
    return commit;
}

static int submoduleAdd(const std::string& repoPath, const std::string& targetPath) {
    // Verificar que el repo existe y es un repo mank
    fs::path rp = fs::path(repoPath);
    if (!fs::exists(rp / ".mank")) {
        Log::error("Not a mank repository: " + repoPath);
        return 1;
    }

    // Verificar que la carpeta destino no existe ya
    if (fs::exists(targetPath)) {
        Log::error("Path already exists: " + targetPath);
        return 1;
    }

    // Verificar que no es ya un submodulo
    auto submodules = Objects::loadSubmodules();
    for (const auto& sub : submodules) {
        if (sub.path == targetPath) {
            Log::error("Submodule already exists at: " + targetPath);
            return 1;
        }
    }

    // Obtener commit actual del submodulo
    std::string commit = getRepoHead(rp);
    if (commit.empty()) {
        Log::error("Could not read HEAD from submodule repo.");
        return 1;
    }

    // Crear symlink a la carpeta del repo
    fs::create_symlink(fs::absolute(rp), targetPath);

    // Registrar el submodulo
    std::string name = fs::path(targetPath).filename().string();
    Objects::Submodule sub;
    sub.name   = name;
    sub.path   = targetPath;
    sub.repo   = fs::absolute(rp).string();
    sub.commit = commit;

    submodules.push_back(sub);
    Objects::saveSubmodules(submodules);

    Log::log("Submodule added: " + name);
    Log::log("  path:   " + targetPath);
    Log::log("  repo:   " + sub.repo);
    Log::log("  commit: " + commit.substr(0, 8) + "...");
    return 0;
}

static int submoduleUpdate() {
    auto submodules = Objects::loadSubmodules();
    if (submodules.empty()) {
        Log::log("No submodules registered.");
        return 0;
    }

    for (auto& sub : submodules) {
        fs::path rp = fs::path(sub.repo);
        if (!fs::exists(rp / ".mank")) {
            Log::warning("Submodule repo not found: " + sub.repo);
            continue;
        }

        std::string newCommit = getRepoHead(rp);
        if (newCommit.empty()) {
            Log::warning("Could not read HEAD for submodule: " + sub.name);
            continue;
        }

        if (newCommit == sub.commit) {
            Log::log(sub.name + ": already up to date (" + newCommit.substr(0, 8) + ")");
            continue;
        }

        sub.commit = newCommit;
        Log::log(sub.name + ": updated to " + newCommit.substr(0, 8) + "...");
    }

    Objects::saveSubmodules(submodules);
    return 0;
}

static int submoduleList() {
    auto submodules = Objects::loadSubmodules();
    if (submodules.empty()) {
        Log::log("No submodules registered.");
        return 0;
    }

    for (const auto& sub : submodules) {
        std::cout << ansi::BOLD << sub.name << ansi::RESET
                  << " -> " << sub.repo
                  << " @ " << ansi::FG_CYAN << sub.commit.substr(0, 8) << ansi::RESET
                  << "\n";
    }
    return 0;
}

static int submoduleRemove(const std::string& targetPath) {
    auto submodules = Objects::loadSubmodules();
    auto it = std::find_if(submodules.begin(), submodules.end(),
        [&](const Objects::Submodule& s) { return s.path == targetPath; });

    if (it == submodules.end()) {
        Log::error("No submodule found at: " + targetPath);
        return 1;
    }

    // Eliminar symlink
    if (fs::exists(targetPath))
        fs::remove(targetPath);

    std::string name = it->name;
    submodules.erase(it);
    Objects::saveSubmodules(submodules);

    Log::log("Submodule removed: " + name);
    return 0;
}

int Mank::submodule(const std::vector<std::string>& args) {
    if (args.empty()) {
        Log::error("Usage: mank submodule <add|update|list|remove>");
        return 1;
    }

    std::string sub = args[0];

    if (sub == "add") {
        if (args.size() < 3) {
            Log::error("Usage: mank submodule add <repo-path> <target-path>");
            return 1;
        }
        return submoduleAdd(args[1], args[2]);
    }

    if (sub == "update") return submoduleUpdate();
    if (sub == "list")   return submoduleList();

    if (sub == "remove") {
        if (args.size() < 2) {
            Log::error("Usage: mank submodule remove <path>");
            return 1;
        }
        return submoduleRemove(args[1]);
    }

    Log::error("Unknown submodule subcommand: " + sub);
    return 1;
}