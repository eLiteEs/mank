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
 */

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <map>

namespace Objects {
	std::string hash(const std::string& content);
	void store(const std::string& hash, const std::string& content);
	std::string read(const std::string& hash);
	std::string createTree(const std::vector<std::pair<std::string,std::string>>& entries);
	std::string createCommit(const std::string& tree, const std::string& parent, const std::string& message);
	std::map<std::string, std::string> readTree(const std::string& hash);
	std::string getLastTree();
	std::map<std::string, std::string> loadIndex();
	std::string readFile(const std::string& path);
	std::string getCurrentBranch();
	std::string getCurrentRef();
	void saveStash(const std::map<std::string, std::string>& files);
	std::map<std::string, std::string> loadStash();
	std::vector<std::string> getHistory(const std::string& branchRef);
	std::string commonAncestor(const std::string& branchA, const std::string& branchB);
	std::string getConfig(const std::string& section, const std::string& key);
	void setConfig(const std::string& section, const std::string& key, const std::string& value, bool global = false);
	bool isRepo();
	std::filesystem::path getRepoRoot();
	void createTag(const std::string& name, const std::string& commitHash);
	std::string getTag(const std::string& name);
	std::vector<std::pair<std::string, std::string>> listTags(); // name, hash

	struct Submodule {
		std::string name;
		std::string path;
		std::string repo;
		std::string commit;
	};

	std::vector<Submodule> loadSubmodules();
	void saveSubmodules(const std::vector<Submodule>& submodules);
}


