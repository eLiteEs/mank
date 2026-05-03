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

#include "man.hpp"

#include <iostream>
#include <string>
#include <string.h>
#include "log.hpp"
#include <fstream>
#include <filesystem>
#include "pager.hpp"

#define BUILDING_CMAKE true

std::string getExecutablePath() {
    if(BUILDING_CMAKE) {
        return std::filesystem::canonical("/proc/self/exe").parent_path().parent_path().string() + "/";
    }
    return std::filesystem::canonical("/proc/self/exe").parent_path().string() + "/";
}

int Man::loadManual(std::string name) {
    if(!std::filesystem::exists(getExecutablePath() + "man/" + name + ".txt")) {
        Log::error("Unknown manual. Use \"mank man\" to get an index of manuals.");
        return 1;
    }

    std::string path = getExecutablePath() + "man/" + name + ".txt";

    std::fstream file(path);
    std::string line;

    Pager::open();

    while(getline(file, line)) {
        std::cout << line << std::endl;
    }

    Pager::close();

    file.close();

    return 0;
}