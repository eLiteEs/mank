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
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <map>
#include <fnmatch.h>

namespace fs = std::filesystem;

static std::vector<std::string> loadIgnore() {
	std::vector<std::string> patterns;
	std::ifstream file(".mankignore");
	if (!file.is_open()) return patterns;

	std::string line;
	while (std::getline(file, line)) {
		if (line.empty() || line[0] == '#') continue;
		if (!line.empty() && line.back() == '/')
			line.pop_back();
		patterns.push_back(line);
	}
	return patterns;
}

static bool isIgnored(const std::string& path, const std::vector<std::string>& patterns) {
	fs::path p = fs::path(path).lexically_normal();
	std::string filename = p.filename().string();

	for (const auto& pattern : patterns) {
		if (fnmatch(pattern.c_str(), filename.c_str(), 0) == 0) return true;
		if (fnmatch(pattern.c_str(), p.string().c_str(), FNM_PATHNAME) == 0) return true;
		for (const auto& component : p) {
			std::string part = component.string();
			if (part == "." || part == "..") continue;
			if (fnmatch(pattern.c_str(), part.c_str(), 0) == 0) return true;
		}
	}
	return false;
}

int Mank::add(const std::string& filepath) {
	auto ignorePatterns = loadIgnore();
	std::string normalizedPath = fs::path(filepath).lexically_normal().string();

	if (fs::is_directory(normalizedPath)) {
		for (const auto& entry : fs::recursive_directory_iterator(normalizedPath)) {
			if (entry.is_regular_file()) {
				std::string path = entry.path().string();
				if (path.find(".mank") != std::string::npos) continue;
				if (isIgnored(path, ignorePatterns)) continue;
				int result = Mank::add(path);
				if (result != 0) return result;
			}
		}
		return 0;
	}

	if (!fs::exists(normalizedPath)) {
		Log::error("File not found: " + normalizedPath);
		return 1;
	}

	std::ifstream file(normalizedPath);
	std::ostringstream ss;
	ss << file.rdbuf();
	std::string content = ss.str();

	std::string h = Objects::hash(content);

	auto index = Objects::loadIndex();
	
	if (index.count(normalizedPath) && index[normalizedPath] == h) {
		return 0;
	}

	Objects::store(h, content);

	index[normalizedPath] = h;
	std::ofstream indexOut(".mank/index", std::ios::trunc);
	for (const auto& [p, hsh] : index)
		indexOut << hsh << " " << p << "\n";

	Log::log("Added: " + normalizedPath + " (" + h.substr(0, 8) + "...)");
	return 0;
}
