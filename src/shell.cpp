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
#include "decorations.hpp"
#include "cli.hpp"
#include "version.hpp"
#include "log.hpp"

#include <iostream>
#include <string>
#include <vector>

// Function I stole from gh:eLiteEs/epi2
std::vector<std::string> splitStringOutsideQuotes(const std::string& str, char c) {
        std::vector<std::string> result;
        std::string temp;
        bool inQuotes = false;
        int parenthesesDepth = 0;

        for (size_t i = 0; i < str.size(); ++i) {
                if (str[i] == '\"') {
                        inQuotes = !inQuotes;
                        temp += str[i];
                } else if(str[i] == '(' && !inQuotes) {
                        parenthesesDepth++;
                        temp += "(";
                } else if(str[i] == ')' && !inQuotes && parenthesesDepth != 0) {
                        parenthesesDepth--;
                        temp += ")";
                } else if (str[i] == c && !inQuotes && parenthesesDepth == 0) {
                        if (!temp.empty()) {
                                result.push_back(temp);
                                temp.clear();
                        }
                } else {
                        temp += str[i];
                }
        }

        if (!temp.empty()) {
                result.push_back(temp);
        }

        return result;
}

// Interactive mank shell
int Mank::shell(const std::vector<std::string>& args) {
	std::cout << ansi::BOLD << "mank " << VERSION << ansi::RESET << " (c) " << YEAR << " Blas Fernández" << std::endl
		  << "Welcome to the interactive shell. Here you can use mank commands directly from this CLI." << std::endl 
		  << "Use \"quit\" to leave." << std::endl;

	if(Objects::isRepo()) {
		std::cout << "Inside repository: " << Objects::getConfig("core", "name") << std::endl
			  << "Latest commit: " << Objects::getHead() << std::endl
			  << "  by: " << Objects::getCommitInfo(Objects::getHead()).user << " <" << Objects::getCommitInfo(Objects::getHead()).email << ">" << std::endl
			  << "  " << Objects::getCommitInfo(Objects::getHead()).title << (Objects::getCommitInfo(Objects::getHead()).description.empty() ? "\n" : "");
	} else {
		std::cout << "Not inside a mank repo. Create one using \"init\"." << std::endl;
	}

	std::cout << std::endl;

	std::string line;

	while(true) {
		std::cout << "mank" << (Objects::isRepo() ? "[" + Objects::getCurrentBranch() + "]" : "") << "> " << ansi::FG_CYAN;
		std::getline(std::cin, line);

		std::cout << ansi::RESET;
		
		if(line == "quit" || line == "q") break;

		std::vector<std::string> parsedCommand = splitStringOutsideQuotes(line, ' ');

		std::string command = parsedCommand[0]; // First argument given to the program
		
		parsedCommand.erase(parsedCommand.begin());

		std::vector<std::string> rest = parsedCommand; // Get the rest of the arguments

		auto it = commands.find(command); // Search for the command
		if(it == commands.end()) { // The search reached the end
		        // Invalid command introduced
		        Log::error("Unknown command: \"" + command + "\".");
		        Log::info("Use \"help\" to get a list of commands for mank.");
		        throw std::exception();

			continue;
		}

		it->second(rest); // Run the command and return its result
	}

	return 0;
}
