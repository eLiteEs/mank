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
#include <filesystem>
#include <zip.h>

namespace fs = std::filesystem;

int Mank::unpack(const std::string& packFile) {
    if (!fs::exists(packFile)) {
        Log::error("Pack file not found: " + packFile);
        return 1;
    }

    int err;
    zip_t* archive = zip_open(packFile.c_str(), ZIP_RDONLY, &err);
    if (!archive) {
        Log::error("Could not open pack file: " + packFile);
        return 1;
    }

    // Detectar si es full (contiene .mank) o source
    bool isFull = false;
    zip_int64_t count = zip_get_num_entries(archive, 0);
    for (zip_int64_t i = 0; i < count; i++) {
        std::string name = zip_get_name(archive, i, 0);
        if (name.substr(0, 5) == ".mank") {
            isFull = true;
            break;
        }
    }

    if (isFull && fs::exists(".mank")) {
        Log::error("There's already a repository here.");
        Log::info("Remove the .mank folder first if you want to overwrite it.");
        zip_close(archive);
        return 1;
    }

    // Extraer todos los archivos
    int extracted = 0;
    for (zip_int64_t i = 0; i < count; i++) {
        std::string name = zip_get_name(archive, i, 0);
        fs::path outPath = fs::current_path() / name;

        // Crear directorios intermedios
        if (outPath.has_parent_path())
            fs::create_directories(outPath.parent_path());

        // Leer y escribir el archivo
        zip_file_t* zf = zip_fopen_index(archive, i, 0);
        if (!zf) continue;

        std::ofstream out(outPath, std::ios::binary);
        char buf[4096];
        zip_int64_t n;
        while ((n = zip_fread(zf, buf, sizeof(buf))) > 0)
            out.write(buf, n);

        zip_fclose(zf);
        extracted++;
    }

    zip_close(archive);

    if (isFull) {
        Log::log("Unpacked full repository (" + std::to_string(extracted) + " files).");
        Log::log("You can now use mank normally in this directory.");
    } else {
        Log::log("Unpacked source (" + std::to_string(extracted) + " files).");
        Log::info("Run \"mank init\" to start tracking this code.");
    }

    return 0;
}