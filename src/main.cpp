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

// Import libs
#include <iostream>
#include <string>
#include "log.hpp"
#include "mank.hpp"
#include "version.hpp"
#include "help.hpp"
#include "objects.hpp"
#include "decorations.hpp"
#include "pager.hpp"
#include "man.hpp"

// Main entry point
int main(int argc, char** argv) {
	// Check if any argument was introduced to the program
	if(argc < 2) {
		// No argument was introduced, show an error
		Log::error("Usage: mank <command>");
		Log::info("If you need help, use \"mank help\"");
		return 1;
	}

	std::string command = argv[1]; // First argument given to the command

	if(command == "help") {
		Pager::open();

		// Show some help to the user
		std::cout << ansi::BOLD << "mank " << VERSION << ansi::RESET<< std::endl
			  << "------------------" << std::endl
			  << "Commands:" << std::endl;

		std::cout << help;

		Pager::close();
			
		return 0;
	} else if(command == "init" || command == "-i") {
		// Check if a directory was introduced
		if(argc < 3) {
			// No directory was introduced
			return Mank::init();
		}

		// A directory was introduced
		std::string dir = argv[2]; // Get the directory introduced
		return Mank::init(dir);
	} else if(command == "version" || command == "-v") {
		// Show the logo of mank
		std::cout << "o o o  mank" << std::endl
			  << "|/ /   " << ansi::FG_CYAN << VERSION << ansi::RESET << std::endl
  			  << "o o" << std::endl
			  << "|/" << std::endl
			  << "o" << std::endl;
		
		// Show current version and some license information
		std::cout << ansi::BOLD <<  "mank " << VERSION << "  Copyright (C) 2026 Blas Fernández" << ansi::RESET << std::endl
			  << "This program comes with " << ansi::BOLD << "ABSOLUTELY NO WARRANTY" << ansi::RESET << "." << std::endl
			  << "This is free software, and you are welcome to redistribute it" << std::endl
			  << "under certain conditions. See LICENSE file for more details." << std::endl
			  << "License: " << ansi::UNDERLINE << "GNU GPL v3" << ansi::RESET << std::endl;

		return 0;
	} else if (command.substr(0, 9) == "--config.") {
		// Local config

		std::string key = command.substr(9); // "user" o "email"
		if (argc < 3) { Log::error("Usage: mank --config." + key + " <value>"); return 1; }
		if(key == "email" || key == "user") {
			return Mank::config("user", key, argv[2]);
		} else if(key == "name") {
			return Mank::config("core", key, argv[2]);
		}
	} else if (command.substr(0, 10) == "--gconfig.") {
		// Global config

		std::string key = command.substr(10); // "user" o "email"
		if (argc < 3) { Log::error("Usage: mank --gconfig." + key + " <value>"); return 1; }
		if(key == "email" || key == "user") {
			return Mank::config("user", key, argv[2], true);
		}
	} else if(command == "man") {
		if(argc < 3) {
			return Man::loadManual("index");
		}

		std::string manual = argv[2];
		return Man::loadManual(manual);
	} else if (!Objects::isRepo()) { // <-------------------------------------- Repository commands
		Log::error("Not inside a mank repository or invalid argument introduced.");
		Log::info("Use \"mank help\" to get some help.");

		return 1;
	} else if(command == "add" || command == "-a") {
		// This command is for adding files to the current commit

		// Check if a file was introduced to add
		if(argc < 3) {
			// Warn the user about this problem
			Log::error("Usage: mank add <file>");
			return 1;
		}

		// Add the file and return the result of the operation
		return Mank::add(argv[2]);
	} else if (command == "commit" || command == "-c") {
		// Commit all the staged files
		if (argc < 3) { Log::error("Usage: mank -c \"message\""); return 1; }
		return Mank::commit(argv[2]);
	} else if (command == "log" || command == "-l") {
		if (argc >= 3 && std::string(argv[2]) == "--oneline")
			return Mank::history(true);
		return Mank::history();
	} else if (command == "status" || command == "-s") {
		// Show which files were changed being staged
		return Mank::status();
	} else if (command == "diff" || command == "-d") {
		if (argc < 3) { Log::error("Usage: mank diff <file|.>"); return 1; }
		if (std::string(argv[2]) == "--commits") {
			if (argc < 5) { Log::error("Usage: mank diff --commits <hash1> <hash2>"); return 1; }
			return Mank::diffCommits(argv[3], argv[4]);
		}
		return Mank::diff(argv[2]);
	} else if (command == "branch" || command == "-b") {
		// Create a new branch
		if (argc < 3) return Mank::branch();
		// Create a new branch using the name that the user gave
		return Mank::branch(argv[2]);
	} else if (command == "switch" || command == "-sw") {
		// Switch to another branch
		if (argc < 3) { Log::error("Usage: mank -sw <branch>"); return 1; }
		return Mank::switchBranch(argv[2]);
	}  else if (command == "restore" || command == "-r") {
		// Delete unstaged changes made to a file
		if (argc < 3) { Log::error("Usage: mank -r <file>"); return 1; }
		return Mank::restore(argv[2]);
	} else if (command == "stash" || command == "-st") {
		// Save file changes for using them later
		if (argc >= 3 && std::string(argv[2]) == "pop")
			// Using pop, get the changes again
			return Mank::stashPop();
		return Mank::stash();
	} else if (command == "merge" || command == "-mg") {
		// Merge another branch with the current branch
		if (argc < 3) { Log::error("Usage: mank -mg <branch>"); return 1; }
		return Mank::merge(argv[2]);
	} else if(command == "dbranch" || command == "-db") {
		// Display in which branch we're
		std::cout << "The current branch is " << ansi::BOLD << Objects::getCurrentBranch() << ansi::BOLD << std::endl;
		return 0;		
	} else if (command == "unstage" || command == "-u") {
		if (argc < 3) { Log::error("Usage: mank unstage <file|.>"); return 1; }
		return Mank::unstage(argv[2]);
	} else if (command == "tag" || command == "-t") {
		if (argc < 3) return Mank::tag();
		return Mank::tag(argv[2]);
	} else if (command == "show") {
		if (argc < 3) { Log::error("Usage: mank show <hash>"); return 1; }
		return Mank::show(argv[2]);
	} else if (command == "checkout" || command == "-co") {
		if (argc < 3) { Log::error("Usage: mank checkout <tag|hash>"); return 1; }
		return Mank::checkout(argv[2]);
	} else if (command == "release") {
		if (argc < 3) { Log::error("Usage: mank release <tag>"); return 1; }
		return Mank::release(argv[2]);
	} else if (command == "pack") {
		if (argc >= 3 && std::string(argv[2]) == "--full")
			return Mank::pack(true);
		return Mank::pack();
	} else if (command == "unpack") {
		if (argc < 3) { Log::error("Usage: mank unpack <file.mank-pack>"); return 1; }
		return Mank::unpack(argv[2]);
	} else if (command == "ci") {
		std::vector<std::string> args;
		for (int i = 2; i < argc; i++)
			args.push_back(argv[i]);
		return Mank::ci(args);
	} else if (command == "submodule" || command == "-sm") {
		std::vector<std::string> args;
		for (int i = 2; i < argc; i++)
			args.push_back(argv[i]);
		return Mank::submodule(args);
	}
}

