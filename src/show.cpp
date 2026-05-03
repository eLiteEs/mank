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
#include "pager.hpp"
#include "decorations.hpp"
#include <iostream>
#include <sstream>

int Mank::show(const std::string& hash) {
    std::string content = Objects::read(hash);
    if (content.empty()) {
        Log::error("Object not found: " + hash);
        return 1;
    }

    // Parsear el commit
    std::istringstream ss(content);
    std::string line;
    std::string tree, parent, date, author, message;

    while (std::getline(ss, line)) {
        if (line.empty()) {
            // Todo lo que queda es el mensaje
            std::ostringstream msg;
            while (std::getline(ss, line))
                msg << line << "\n";
            message = msg.str();
            break;
        }
        if (line.substr(0, 5) == "tree ")   tree   = line.substr(5);
        if (line.substr(0, 7) == "parent ") parent = line.substr(7);
        if (line.substr(0, 5) == "date ")   date   = line.substr(5);
        if (line.substr(0, 7) == "author ") author = line.substr(7);
    }

    Pager::open();

    // Header del commit
    std::cout << ansi::BOLD << ansi::FG_YELLOW << "commit " << hash << ansi::RESET << "\n";
    if (!author.empty())
        std::cout << ansi::BOLD << "Author: " << ansi::RESET << author << "\n";
    if (!date.empty()) {
        std::time_t t = std::stol(date);
        std::cout << ansi::BOLD << "Date:   " << ansi::RESET << std::ctime(&t);
    }
    if (!parent.empty())
        std::cout << ansi::BOLD << "Parent: " << ansi::RESET << parent.substr(0, 8) << "...\n";
    std::cout << "\n    " << message << "\n";

    // Mostrar el diff respecto al parent
    if (!parent.empty()) {
        auto treeNew = Objects::readTree(tree);
        std::string parentContent = Objects::read(parent);
        std::string parentTree;
        std::istringstream ps(parentContent);
        while (std::getline(ps, line))
            if (line.substr(0, 5) == "tree ") parentTree = line.substr(5);

        auto treeOld = Objects::readTree(parentTree);

        // Archivos modificados o añadidos
        for (const auto& [path, newHash] : treeNew) {
            std::string oldContent, newContent;
            newContent = Objects::read(newHash);

            if (treeOld.count(path)) {
                oldContent = Objects::read(treeOld[path]);
                if (oldContent == newContent) continue; // sin cambios
            }

            std::cout << ansi::BOLD << "--- " << path << " (before)\n";
            std::cout << "+++ " << path << " (after)\n" << ansi::RESET;
            printDiff(splitLines(oldContent), splitLines(newContent));
            std::cout << "\n";
        }

        // Archivos eliminados
        for (const auto& [path, oldHash] : treeOld) {
            if (!treeNew.count(path)) {
                std::cout << ansi::BOLD << ansi::FG_RED << "deleted: " << path << ansi::RESET << "\n";
            }
        }
    }

    Pager::close();
    return 0;
}