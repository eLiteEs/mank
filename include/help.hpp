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

#pragma once

#include <string>
#include "decorations.hpp"

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

#pragma once

#include <string>
#include "decorations.hpp"

std::string help = "" +
std::string(ansi::BOLD) + "mank help" + std::string(ansi::RESET) + " >> Show this help.\n" +
std::string(ansi::BOLD) + "mank init | -i [directory]" + std::string(ansi::RESET) + " >> Initialize a mank repository in the current directory or in a specific folder.\n" +
std::string(ansi::BOLD) + "mank version" + std::string(ansi::RESET) + " >> Show mank's version\n" +
std::string(ansi::BOLD) + "mank add | -a <file>" + std::string(ansi::RESET) + " >> Stage a file. Use \".\" for adding all the files and folders recursively.\n" +
std::string(ansi::BOLD) + "mank commit | -c <message>" + std::string(ansi::RESET) + " >> Make a commit.\n" +
std::string(ansi::BOLD) + "mank log | -l" + std::string(ansi::RESET) + " >> Display the commit history.\n" +
std::string(ansi::BOLD) + "mank log --oneline" + std::string(ansi::RESET) + " >> Display compact commit history.\n" +
std::string(ansi::BOLD) + "mank status | -s" + std::string(ansi::RESET) + " >> Show the status of staged and unstaged changes.\n" +
std::string(ansi::BOLD) + "mank diff | -d <file>" + std::string(ansi::RESET) + " >> Show the diff of an specific file. Use \".\" for watching the diff of all the files of the repository.\n" +
std::string(ansi::BOLD) + "mank diff --commits <hash1> <hash2>" + std::string(ansi::RESET) + " >> Compare two commits.\n" +
std::string(ansi::BOLD) + "mank branch | -b <name>" + std::string(ansi::RESET) + " >> Create a branch with an specific name.\n" +
std::string(ansi::BOLD) + "mank switch | -sw <branch>" + std::string(ansi::RESET) + " >> Switch into another branch.\n" +
std::string(ansi::BOLD) + "mank restore | -r <file>" + std::string(ansi::RESET) + " >> Discard unstaged changes from a file.\n" +
std::string(ansi::BOLD) + "mank stash | -st" + std::string(ansi::RESET) + " >> Save the changes in the current repository for later.\n" +
std::string(ansi::BOLD) + "mank stash | -st pop" + std::string(ansi::RESET) + " >> Load stashed changes.\n" +
std::string(ansi::BOLD) + "mank merge | -mg <branch>" + std::string(ansi::RESET) + " >> Merge a branch with the current branch.\n" +
std::string(ansi::BOLD) + "mank dbranch | -db" + std::string(ansi::RESET) + " >> Display the current branch.\n" +
std::string(ansi::BOLD) + "mank --config.<config> <value>" + std::string(ansi::RESET) + " >> Change a local configuration to another value\n" +
std::string(ansi::BOLD) + "mank --gconfig.<config> <value>" + std::string(ansi::RESET) + " >> Change a global configuration to another value\n" +
std::string(ansi::BOLD) + "mank --rconfig.<config> <value>" + std::string(ansi::RESET) + " >> Change a repository core configuration to another value\n" +
std::string(ansi::BOLD) + "mank --config.name <name>" + std::string(ansi::RESET) + " >> Set your name for local commits.\n" +
std::string(ansi::BOLD) + "mank --config.email <name>" + std::string(ansi::RESET) + " >> Set your email for local commits.\n" +
std::string(ansi::BOLD) + "mank unstage | -u <file>" + std::string(ansi::RESET) + " >> Unstage a file.\n" +
std::string(ansi::BOLD) + "mank tag | -t [name]" + std::string(ansi::RESET) + " >> Create a tag on the current commit, or list all tags.\n" +
std::string(ansi::BOLD) + "mank checkout | -co <tag|hash>" + std::string(ansi::RESET) + " >> Restore working tree to a specific commit or tag.\n" +
std::string(ansi::BOLD) + "mank release <tag>" + std::string(ansi::RESET) + " >> Generate a release from a tag with source zip, changelog and HTML.\n" +
std::string(ansi::BOLD) + "mank --config.name <name>" + std::string(ansi::RESET) + " >> Change the name of the repository.\n" +
std::string(ansi::BOLD) + "mank pack [--full]" + std::string(ansi::RESET) + " >> Pack the source of the last commit. Use --full to include the entire history.\n" +
std::string(ansi::BOLD) + "mank unpack <file.mank-pack>" + std::string(ansi::RESET) + " >> Unpack a mank pack file in the current directory.\n" +
std::string(ansi::BOLD) + "mank ci run [job]" + std::string(ansi::RESET) + " >> Run all pipeline jobs or a specific one.\n" +
std::string(ansi::BOLD) + "mank ci logs [job] [--all]" + std::string(ansi::RESET) + " >> Show logs of all jobs or a specific one. Use --all for full history.\n" +
std::string(ansi::BOLD) + "mank submodule add <repo> <path>" + std::string(ansi::RESET) + " >> Link a mank repo as a submodule.\n" +
std::string(ansi::BOLD) + "mank submodule update" + std::string(ansi::RESET) + " >> Update all submodules to their latest commit.\n" +
std::string(ansi::BOLD) + "mank submodule list" + std::string(ansi::RESET) + " >> List all submodules.\n" +
std::string(ansi::BOLD) + "mank submodule remove <path>" + std::string(ansi::RESET) + " >> Remove a submodule.\n";
