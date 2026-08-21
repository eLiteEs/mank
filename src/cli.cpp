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

#include "cli.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "log.hpp"
#include "mank.hpp"
#include "version.hpp"
#include "help.hpp"
#include "objects.hpp"
#include "decorations.hpp"
#include "pager.hpp"
#include "man.hpp"

namespace Command {

// Show some help to the user
int help(const std::vector<std::string>& args) {	
        std::cout << ansi::BOLD << "mank " << VERSION << ansi::RESET<< std::endl
	        << "------------------" << std::endl
                << "Commands:" << std::endl;

        std::cout << help_list;

	Pager::close();
        return 0;
}

// Initialize a mank repository
int init(const std::vector<std::string>& args) {
        // Check if a directory was introduced
        if(args.size() < 2) {
                // No directory was introduced
                return Mank::init();
        }

        // A directory was introduced
        std::string dir = args[0]; // Get the directory introduced
        return Mank::init(dir);
}

// Show mank version
int version(const std::vector<std::string>& args) {
	// Show the logo of mank
        std::cout << "o o o  mank" << std::endl
                << "|/ /   " << ansi::FG_CYAN << VERSION << ansi::RESET << std::endl
                << "o o" << std::endl
        	<< "|/" << std::endl
                << "o" << std::endl;

	// Show current version and some license information
	std::cout << ansi::BOLD <<  "mank " << ansi::BLINK << VERSION << ansi::RESET << ansi::BOLD << "  Copyright (C) " << YEAR << " Blas Fern�ndez" << ansi::RESET << std::endl
                << "This program comes with " << ansi::BOLD << "ABSOLUTELY NO WARRANTY" << ansi::RESET << "." << std::endl
          	<< "This is free software, and you are welcome to redistribute it" << std::endl
                << "under certain conditions. See LICENSE file for more details." << std::endl
                << "License: " << ansi::UNDERLINE << "GNU GPL v3" << ansi::RESET << std::endl;

	return 0;
}

// Modify a mank config
int config(const std::vector<std::string>& args) {
	// Check if the minimun amount of arguments has been introduced
	// Usage: mank config key value [--global]

	if(args.size() < 2) {
		std::cout << ansi::FG_RED << "Invalid usage of command." << ansi::RESET << std::endl
			  << "Usage: mank config <key> <value> [--global]" << std::endl;
		return 1;
	}

	std::string key = args[0];

	if(key == "user.email" || key == "user.name") {
		// Check if the user used the --global argument
		if(args.size() >= 3 && args[2] == "--global") return Mank::config("user", key == "user.email" ? "email" : "name", args[1], true);	

		// Check if the user is inside a mank repo
		if(!Objects::isRepo()) {
			std::cout << ansi::FG_RED << "You need to be inside a mank repo to modify local configs." << ansi::RESET << std::endl;
			return 1;
		}


		// Just write on local config
		return Mank::config("user", key == "user.email" ? "email" : "name", args[1]);
	} else if(key == "repo.name") {
		// Check if the user is not inside a mank repo
		if(!Objects::isRepo()) {
			std::cout << "To change the name of a repository, you must be inside one!" << ansi::RESET << std::endl;
			return 1;
		}

		// Write the config
		return Mank::config("core", "name", args[1]);
	}

	std::cout << ansi::FG_RED << "Invalid config key. Use \"user.email\", \"user.name\" or \"repo.name\"." << ansi::RESET << std::endl;
	return 1;
}

// Manuals
int man(const std::vector<std::string>& args) {
	// Check if the user specified a manual
	if(args.empty()) {
		// Open index of manuals
		return Man::loadManual("index");
	}

	// Open the desired manual
	std::string manual = args[0];
	return Man::loadManual(manual);
}

// Add a file to tracking
int add(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user introduced a file to add
	if(args.empty()) {
		std::cout << ansi::FG_RED << "Usage: mank add <file>" << ansi::RESET << std::endl;
		return 1;
	}

	// Add the file
	return Mank::add(args[0]);
} 

// Commit files
int commit(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	std::string msg;

	// Check if the user included a commit message
	if(args.empty()) {
		// If he didn't, ask him to add one
		std::cout << "Include a message for the commit: " << ansi::BOLD;
		std::getline(std::cin, msg);
		
		// Check if the user wants to add a description
		std::cout << ansi::RESET << "Do you want to add a description? (yes or no): " << ansi::BOLD;
		std::string confirmation;
		std::getline(std::cin, confirmation);

		if(confirmation == "yes") {
			std::cout << "Write the description here. Create a line with just \"EOF\" to complete it." << ansi::RESET << std::endl;

			std::string line, description;
			while(true) {
				std::getline(std::cin, line);
			
				if(line == "EOF") {
					break;
				}

				description += line + "\n";
			}

			msg += "&newline&" + description;
		}
	} else {
		// If it did, get it
		msg = args[0];

		if(args.size() >= 2) { // The user added a description via CLI
			msg += "&newline&" + args[1];
		}
	}

	// Check if the message is empty
	if(msg.empty()) {
		std::cout << ansi::FG_RED << "The commit message shouldn't be empty." << ansi::RESET << std::endl;
		return 1;
	}

	return Mank::commit(msg);
}

// History of commits
int log(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user used --oneline command
	if (!args.empty() && args[0] == "--oneline")
                return Mank::history(true); // Show the history with one line by commit
        return Mank::history(); // Show the normal history
}

// Repo status
int status(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	return Mank::status();
}

// Show the difference in changes
int diff(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user specified any file
	if(args.empty()) {
		return Mank::diff("."); 
	}

	// Check if the user wants to compare two commits
	if(args[0] == "--commits") {
		if(args.size() < 3) { // Check if the user included the neccesary arguments
			std::cout << ansi::FG_RED << "Usage: mank diff --commits <hash1> <hash2>" << ansi::RESET << std::endl;
			return 1;
		}

		std::string second = args[2];	

		if(second == "latest") {
			second = Objects::getHead();
		} 
	
		return Mank::diffCommits(args[1], second);
	}

	return Mank::diff(args[0]);
}

// Create branchs
int branch(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user introduced a name for the branch
	if(args.empty()) {
		// It didn't so display the current branch name
		std::cout << "The current branch is " << ansi::BOLD << Objects::getCurrentBranch() << ansi::RESET << "." << std::endl;
		return Mank::branch();
	}

	// Just create the branch with the name provided
	return Mank::branch(args[0]);
}

// Switch into another branch
int switchBranch(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user specified a branch to change into
	if(args.empty()) {
		std::cout << ansi::FG_RED << "You should specify a branch name to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	return Mank::switchBranch(args[0]);
}

// Delete unstaged changes
int restore(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user introuced the minimun amout of arguments
	if(args.empty()) {
		std::cout << ansi::FG_RED << "Usage: mank restore <file>" << ansi::RESET << std::endl;
		return 1;
	}

	return Mank::restore(args[0]);
}

// Save unstaged changes for later
int stash(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	if(args.size() >= 1 && args[0] == "pop") { // With pop, recover the changes
		return Mank::stashPop();
	}

	return Mank::stash();
}

// Merge a branch
int merge(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user included a branch name.
	if(args.empty()) {
		std::cout << ansi::FG_RED << "For merging a branch, you should include its name. Usage: mank merge <arrival branch>" << ansi::RESET << std::endl;
		return 1;
	}

	return Mank::merge(args[0]);
}

// Unstage a file
int unstage(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user selected a file to unstage
	if(args.empty()) {
		std::cout << ansi::FG_RED << "You must include a file to unstageto use this command. Usage: mank unstage <file>" << ansi::RESET << std::endl;
		return 1;
	}

	return Mank::unstage(args[0]);
}

// Create a tag
int tag(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user introduced any argument
	if(args.empty()) {
		// As no argument was given, list current tags
		return Mank::tag();
	}

	// Use the first argument as name for the tag
	return Mank::tag(args[0]);
}

// Show detailed information about an specific commit
int show(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user introduced any argument
	if(args.empty()) {
		std::cout << ansi::FG_RED << "You should specify a commit hash in order to see information about it." << ansi::RESET << std::endl;
		return 1;
	}

	std::cout << "!" << std::endl;

	return Mank::show(args[0]);
}

// Return to an specific commit
int checkout(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user introduced any argument
	if(args.empty()) {
		std::cout << ansi::FG_RED << "You should include a commit hash or tag to checkout. Usage: mank checkout <hash|tag>" << ansi::RESET << std::endl;
		return 1;
	}

	return Mank::checkout(args[0]);
}

// Make a release
int release(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	// Check if the user introduced any argument
	if(args.empty()) {
		std::cout << ansi::FG_RED << "You should include a tag to make a commit from that. Usage: mank release <tag>" << ansi::RESET << std::endl;
		return 1;
	}	

	return Mank::release(args[0]);
}

// Pack a repository
int pack(const std::vector<std::string>& args) {
	// As you need to be inside a mank repo to use this command, check it
	if(!Objects::isRepo()) {
		std::cout << ansi::FG_RED << "You must be inside a mank repo to use this command." << ansi::RESET << std::endl;
		return 1;
	}

	if(!args.empty() && args[0] == "--full") { // Check if the user used the full argumento to make a complete pack
		return Mank::pack(true);
	}

	// Do a normal pack (don't include history)
	return Mank::pack();
}

// Unpack a repository
int unpack(const std::vector<std::string>& args) {
	if(args.empty()) { // Check if the user didn't specified a pack file
		std::cout << ansi::FG_RED << "You must specify a .mank-pack file. Usage: mank unpack <file>" << ansi::RESET << std::endl;
	}

	return Mank::unpack(args[0]);
}

// CI/CD
int ci(const std::vector<std::string>& args) {
	return Mank::ci(args);
}

// Submodules
int submodule(const std::vector<std::string>& args) {
	return Mank::submodule(args);
}

}
