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
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <zip.h>

namespace fs = std::filesystem;

int Mank::pack(bool full) {
    // Obtener nombre del repo
    std::string repoName = Objects::getConfig("core", "name");
    if (repoName.empty())
        repoName = fs::current_path().filename().string();

    std::string packName = repoName + ".mank-pack";
    fs::path packPath = fs::current_path() / packName;

    int err;
    zip_t* archive = zip_open(packPath.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!archive) {
        Log::error("Could not create pack file.");
        return 1;
    }

    if (full) {
        // ── Modo full: empaquetar todo .mank ──────────────────
        fs::path mankDir = fs::path(".mank");
        for (const auto& entry : fs::recursive_directory_iterator(mankDir)) {
            if (!entry.is_regular_file()) continue;

            std::string rel = fs::relative(entry.path()).string();

            std::ifstream f(entry.path(), std::ios::binary);
            std::ostringstream ss;
            ss << f.rdbuf();
            std::string content = ss.str();

            zip_source_t* src = zip_source_buffer(archive, content.c_str(), content.size(), 0);
            if (src)
                zip_file_add(archive, rel.c_str(), src, ZIP_FL_OVERWRITE);
        }
        Log::log("Full pack created: " + packName);
        Log::log("Contains full history and all objects.");
    } else {
        // ── Modo source: solo el ultimo commit ─────────────────
        std::string treeHash = Objects::getLastTree();
        if (treeHash.empty()) {
            Log::error("No commits yet.");
            zip_close(archive);
            fs::remove(packPath);
            return 1;
        }

        auto tree = Objects::readTree(treeHash);
        if (tree.empty()) {
            Log::error("Empty tree.");
            zip_close(archive);
            fs::remove(packPath);
            return 1;
        }

        for (const auto& [path, hash] : tree) {
            std::string content = Objects::read(hash);
            if (content.empty()) continue;

            zip_source_t* src = zip_source_buffer(archive, content.c_str(), content.size(), 0);
            if (src)
                zip_file_add(archive, path.c_str(), src, ZIP_FL_OVERWRITE);
        }
        Log::log("Pack created: " + packName);
        Log::log("Contains source from last commit.");
    }

    zip_close(archive);
    Log::log("Saved to: " + packPath.string());
    return 0;
}