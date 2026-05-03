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
#include <fstream>
#include <iostream>

int Mank::checkout(const std::string& target) {
    // Resolver si es una tag o un hash directo
    std::string commitHash = Objects::getTag(target);
    if (commitHash.empty())
        commitHash = target; // asumir que es un hash directo

    // Verificar que el commit existe
    std::string content = Objects::read(commitHash);
    if (content.empty()) {
        Log::error("Commit not found: " + target);
        return 1;
    }

    // Extraer el tree del commit
    std::string treeHash;
    std::istringstream ss(content);
    std::string line;
    while (std::getline(ss, line))
        if (line.substr(0, 5) == "tree ")
            treeHash = line.substr(5);

    if (treeHash.empty()) {
        Log::error("Could not read tree from commit.");
        return 1;
    }

    auto tree = Objects::readTree(treeHash);
    if (tree.empty()) {
        Log::error("Empty tree.");
        return 1;
    }

    // Restaurar cada archivo del tree
    auto index = Objects::loadIndex();
    int restored = 0;

    for (const auto& [path, hash] : tree) {
        std::string fileContent = Objects::read(hash);
        if (fileContent.empty()) continue;

        // Crear directorios intermedios si hacen falta
        std::filesystem::path p(path);
        if (p.has_parent_path())
            std::filesystem::create_directories(p.parent_path());

        std::ofstream out(path);
        out << fileContent;

        index[path] = hash;
        restored++;
    }

    // Actualizar índice
    std::ofstream indexOut(".mank/index", std::ios::trunc);
    for (const auto& [p, h] : index)
        indexOut << h << " " << p << "\n";

    Log::log("Checked out " + std::to_string(restored) + " files from " + commitHash.substr(0, 8) + "...");
    return 0;
}