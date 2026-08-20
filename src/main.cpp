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

// Let me cook now
// gl trying to undestand anything here
// i guess this maybe is going to be abandoned soon
// but let my try it
// 19-04-2026

// Dayum ts is cool
// maybe i should do this more frequently
// damn my english is criminal
// linus torvalds did git for fun
// facts.
// 26-05-2026

// I thought it was going to be harder xd
// 2-05-2026

// oh shet
// there we go again
// 18-08-2025

// Import libs
#include <iostream>
#include <string>

#include "log.hpp"
#include "cli.hpp"

// Main entry point
int main(int argc, char** argv) {
	// Check if any argument was introduced to the program
	if(argc < 2) {
		// No argument was introduced, show an error
		Log::error("Usage: mank <command>");
		Log::info("If you need help, use \"mank help\"");
		return 1;
	}

	std::string command = argv[1]; // First argument given to the program
	std::vector<std::string> rest(argv + 2, argv + argc); // Get the rest of the arguments

	auto it = commands.find(command); // Search for the command
	if(it == commands.end()) { // The search reached the end
		// Invalid command introduced
		Log::error("Unknown command: \"" + command + "\".");
		Log::info("Use \"mank help\" to get a list of commands for mank.");
		return 1;
	}

	return it->second(rest); // Run the command and return its result
}

