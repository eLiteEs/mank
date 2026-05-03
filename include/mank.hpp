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
#include <vector>

struct Mank {
	static int init(std::string dir = ".");
	static int add(const std::string& filepath);
	static int commit(const std::string& message);
	static int history(bool oneline = false);
	static int status();
	static int diff(const std::string& filepath);
	static int branch(const std::string& name = "");
	static int switchBranch(const std::string& name);
	static int restore(const std::string& filepath);
	static int stash();
	static int stashPop();
	static int merge(const std::string& branchName);
	static int config(const std::string& section, const std::string& key, const std::string& value, bool global = false);
	static int unstage(const std::string& filepath);	
	static int tag(const std::string& name = "");
	static int show(const std::string& hash);
	static int diffCommits(const std::string& hashA, const std::string& hashB);
	static int checkout(const std::string& target);
	static int release(const std::string& tag);
	static int pack(bool full = false);
	static int unpack(const std::string& packFile);
	static int ci(const std::vector<std::string>& args, const std::string& trigger = "");
	static int submodule(const std::vector<std::string>& args);
};

std::vector<std::string> splitLines(const std::string& s);
void printDiff(const std::vector<std::string>& a, const std::vector<std::string>& b);
