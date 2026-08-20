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

#include <iostream>
#include <fstream>
#include <algorithm>
#include <sstream>

#include "objects.hpp"
#include "log.hpp"
#include "pager.hpp"
#include "decorations.hpp"

int Mank::show(const std::string& hash) {
	std::string content = Objects::read(hash);
	if (content.empty()) {
		Log::error("Object not found: " + hash);
		return 1;
	}

	std::istringstream ss(content);
	std::string line;
	std::string tree, parent, date, author, email, message;

	while (std::getline(ss, line)) {
		if (line.empty()) {
			std::ostringstream msg;
			while (std::getline(ss, line))
				msg << line << "\n";
			message = msg.str();
			break;
		}
		if (line.substr(0, 5) == "tree ")   tree   = line.substr(5);
		if (line.substr(0, 7) == "parent ") parent = line.substr(7);
		if (line.substr(0, 5) == "date ")   date   = line.substr(5);
		if (line.substr(0, 3) == "by ")	 author = line.substr(3);
		if (line.substr(0, 6) == "email ")  email  = line.substr(6);
	}

	Pager::open();

	std::cout << ansi::BOLD << ansi::FG_YELLOW << "commit " << hash 
		  << ansi::RESET << "\n";
	
	if (!author.empty())
		std::cout << ansi::BOLD << "Author: " << ansi::RESET 
			  << author;
	if (!email.empty())
		std::cout << " <" << email << ">";
	std::cout << "\n";
	
	if (!date.empty()) {
		std::time_t t = std::stol(date);
		std::cout << ansi::BOLD << "Date:   " << ansi::RESET 
			  << std::ctime(&t);
	}

	/*
	std::cout << ansi::BOLD << "Branches:" << ansi::RESET << " ";
	bool foundBranch = false;
	for (const auto& entry : std::filesystem::directory_iterator(".mank/refs/heads")) {
		std::ifstream ref(entry.path());
		std::string branchHash;
		std::getline(ref, branchHash);
		if (branchHash == hash || 
			Objects::getHistory(entry.path().string())
				.end() != std::find(Objects::getHistory(entry.path().string()).begin(),
						    Objects::getHistory(entry.path().string()).end(),
						    hash)) {
			std::cout << ansi::FG_GREEN << entry.path().filename().string() 
					  << ansi::RESET << " ";
			foundBranch = true;
		}
	}
	if (!foundBranch) std::cout << "(none)";
	std::cout << "\n";
	*/

	auto tags = Objects::listTags();
	std::cout << ansi::BOLD << "Tags:	" << ansi::RESET;
	bool foundTag = false;
	for (const auto& [name, tagHash] : tags) {
		if (tagHash == hash) {
			std::cout << ansi::FG_CYAN << name << ansi::RESET << " ";
			foundTag = true;
		}
	}
	if (!foundTag) std::cout << "(none)";
	std::cout << "\n\n";

	std::cout << "	" << message << "\n";

	if (!parent.empty()) {
		auto treeNew = Objects::readTree(tree);
		
		std::string parentContent = Objects::read(parent);
		std::string parentTree;
		std::istringstream ps(parentContent);
		while (std::getline(ps, line))
			if (line.substr(0, 5) == "tree ") 
				parentTree = line.substr(5);

		auto treeOld = Objects::readTree(parentTree);

		int filesAdded = 0, filesDeleted = 0, filesModified = 0;
		int totalAdditions = 0, totalDeletions = 0;

		for (const auto& [path, newHash] : treeNew) {
			std::string newContent = Objects::read(newHash);
			std::string oldContent;

			if (treeOld.count(path)) {
				oldContent = Objects::read(treeOld[path]);
				if (oldContent == newContent) continue;
				
				auto oldLines = splitLines(oldContent);
				auto newLines = splitLines(newContent);
				int adds = 0, dels = 0;
				for (const auto& l : newLines)
					if (std::find(oldLines.begin(), oldLines.end(), l) 
					    == oldLines.end()) adds++;
				for (const auto& l : oldLines)
					if (std::find(newLines.begin(), newLines.end(), l) 
					    == newLines.end()) dels++;
				
				totalAdditions += adds;
				totalDeletions += dels;
				filesModified++;
			} else {
				auto newLines = splitLines(newContent);
				totalAdditions += newLines.size();
				filesAdded++;
			}
		}

		for (const auto& [path, oldHash] : treeOld) {
			if (!treeNew.count(path)) {
				auto oldLines = splitLines(Objects::read(oldHash));
				totalDeletions += oldLines.size();
				filesDeleted++;
			}
		}

		std::cout << "\n";
		setColor(32, 180, 220);
		std::cout << ansi::BOLD << "Summary:" << ansi::RESET << "\n";
		resetColor();
		
		std::cout << "  Files: ";
		if (filesModified > 0) std::cout << filesModified << " modified";
		if (filesAdded > 0) {
			if (filesModified > 0) std::cout << ", ";
			std::cout << ansi::FG_GREEN << filesAdded << " new" 
					  << ansi::RESET;
		}
		if (filesDeleted > 0) {
			if (filesModified > 0 || filesAdded > 0) std::cout << ", ";
			std::cout << ansi::FG_RED << filesDeleted << " deleted" 
					  << ansi::RESET;
		}
		std::cout << "\n";
		
		std::cout << "  Changes: ";
		if (totalAdditions > 0) 
			std::cout << ansi::FG_GREEN << "+" << totalAdditions 
					  << ansi::RESET;
		if (totalAdditions > 0 && totalDeletions > 0) 
			std::cout << " ";
		if (totalDeletions > 0) 
			std::cout << ansi::FG_RED << "-" << totalDeletions 
					  << ansi::RESET;
		std::cout << "\n\n";

		std::cout << ansi::BOLD << "Changed files:" 
				  << ansi::RESET << "\n";
		
		for (const auto& [path, newHash] : treeNew) {
			if (!treeOld.count(path)) {
				std::cout << "  " << ansi::FG_GREEN << "[new]" 
					      << ansi::RESET << "  " << path << "\n";
			} else if (Objects::read(treeOld[path]) != 
					   Objects::read(newHash)) {
				std::cout << "  " << ansi::FG_YELLOW << "[mod]  " 
					      << ansi::RESET << "  " << path << "\n";
			}
		}
		for (const auto& [path, _] : treeOld) {
			if (!treeNew.count(path)) {
				std::cout << "  " << ansi::FG_RED << "[del] " 
					      << ansi::RESET << "  " << path << "\n";
			}
		}
		std::cout << "\n";

		std::cout << ansi::BOLD << "Detailed diff:" 
				  << ansi::RESET << "\n\n";
		
		for (const auto& [path, newHash] : treeNew) {
			std::string newContent = Objects::read(newHash);
			std::string oldContent;

			if (treeOld.count(path)) {
				oldContent = Objects::read(treeOld[path]);
				if (oldContent == newContent) continue;
			}

			std::cout << ansi::BOLD << "--- " << path << " (before)\n";
			std::cout << "+++ " << path << " (after)\n" << ansi::RESET;
			printDiff(splitLines(oldContent), splitLines(newContent),
					  path, oldContent, newContent);
			std::cout << "\n";
		}

		for (const auto& [path, oldHash] : treeOld) {
			if (!treeNew.count(path)) {
				std::cout << ansi::BOLD << ansi::FG_RED 
					  << "removed: " << path << ansi::RESET << "\n";
			}
		}
	}

	Pager::close();
	return 0;
}
