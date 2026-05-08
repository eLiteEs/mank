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

#include "objects.hpp"
#include <openssl/sha.h>
#include <zlib.h>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <iomanip>
#include <vector>
#include <ctime>
#include <map>
#include <set>

namespace fs = std::filesystem;

// Calculate the SHA-256 and return the result in hex 
std::string Objects::hash(const std::string& content) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(content.c_str()), 
           content.size(), digest);

    std::ostringstream oss;
    // Only the first 8 bytes = 16 hex characters
    for (int i = 0; i < 8; i++)
        oss << std::hex << std::setw(2) << std::setfill('0') 
            << (int)digest[i];

    return oss.str();  // 16 characters instead of 64
}

// Compress and save the content in .mank/objects/
void Objects::store(const std::string& hash, const std::string& content) {
	// Compress with zlib
	uLongf compSize = compressBound(content.size());
	std::vector<Bytef> compressed(compSize);
	compress(compressed.data(), &compSize,
			 reinterpret_cast<const Bytef*>(content.c_str()), content.size());

	// Create path: .mank/objects/<2 chars>/<rest>
	fs::path dir = fs::path(".mank") / "objects" / hash.substr(0, 2);
	fs::create_directories(dir);
	fs::path file = dir / hash.substr(2);

	// Write 
	std::ofstream out(file, std::ios::binary);
	out.write(reinterpret_cast<char*>(compressed.data()), compSize);
}

std::string Objects::createTree(const std::vector<std::pair<std::string,std::string>>& entries) {
	std::ostringstream ss;
	for (const auto& [hash, path] : entries)
		ss << "blob " << hash << " " << path << "\n";
	
	std::string content = ss.str();
	std::string h = Objects::hash(content);
	Objects::store(h, content);
	return h;
}

std::string Objects::createCommit(const std::string& tree, const std::string& parent, const std::string& message) {
	std::string name  = getConfig("user", "name");
	std::string email = getConfig("user", "email");
	
	std::ostringstream ss;
	ss << "tree " << tree << "\n";
	if (!parent.empty())
		ss << "parent " << parent << "\n";
	ss << "date " << std::time(nullptr) << "\n";
	ss << "by " << name << "\n";
	ss << "email " << email << "\n";
	ss << "\n" << message << "\n";

	std::string content = ss.str();
	std::string h = Objects::hash(content);
	Objects::store(h, content);
	return h;
}

std::string Objects::read(const std::string& hash) {
	fs::path file = fs::path(".mank") / "objects" / hash.substr(0, 2) / hash.substr(2);

	if (!fs::exists(file)) return "";

	// Read compresssed
	std::ifstream in(file, std::ios::binary);
	std::vector<Bytef> compressed((std::istreambuf_iterator<char>(in)), {});

	// Uncompress
	uLongf size = compressed.size() * 10;
	std::vector<Bytef> decompressed(size);
	uncompress(decompressed.data(), &size, compressed.data(), compressed.size());

	return std::string(reinterpret_cast<char*>(decompressed.data()), size);
}

std::map<std::string, std::string> Objects::readTree(const std::string& hash) {
	std::map<std::string, std::string> entries; // path -> hash
	std::string content = Objects::read(hash);
	if (content.empty()) return entries;

	std::istringstream ss(content);
	std::string line;
	while (std::getline(ss, line)) {
		// formato: "blob <hash> <path>"
		if (line.substr(0, 5) != "blob ") continue;
		std::string rest = line.substr(5);
		size_t space = rest.find(' ');
		std::string h = rest.substr(0, space);
		std::string p = rest.substr(space + 1);
		entries[p] = h;
	}
	return entries;
}

std::string Objects::getLastTree() {
	std::ifstream refFile(getCurrentRef());
	if (!refFile.is_open()) return "";

	std::string commitHash;
	std::getline(refFile, commitHash);
	if (commitHash.empty()) return "";

	std::string content = Objects::read(commitHash);
	std::istringstream ss(content);
	std::string line;
	while (std::getline(ss, line)) {
		if (line.substr(0, 5) == "tree ")
			return line.substr(5);
	}
	return "";
}

std::map<std::string, std::string> Objects::loadIndex() {
	std::map<std::string, std::string> index;
	std::ifstream file(".mank/index");
	if (!file.is_open()) return index;

	std::string hash, path;
	while (file >> hash >> path)
		index[path] = hash;

	return index;
}

std::string Objects::readFile(const std::string& path) {
	std::string treeHash = getLastTree();
	if (treeHash.empty()) return "";

	auto tree = readTree(treeHash);
	if (!tree.count(path)) return "";

	return read(tree[path]);
}

std::string Objects::getCurrentBranch() {
	std::ifstream head(".mank/HEAD");
	if (!head.is_open()) return "";

	std::string line;
	std::getline(head, line);

	// formato: "ref: refs/heads/main"
	const std::string prefix = "ref: refs/heads/";
	if (line.substr(0, prefix.size()) == prefix)
		return line.substr(prefix.size());

	return ""; // detached HEAD
}

std::string Objects::getCurrentRef() {
	std::string branch = getCurrentBranch();
	if (branch.empty()) return "";
	return ".mank/refs/heads/" + branch;
}

void Objects::saveStash(const std::map<std::string, std::string>& files) {
	std::ofstream stash(".mank/stash", std::ios::trunc);
	for (const auto& [path, hash] : files)
		stash << hash << " " << path << "\n";
}

std::map<std::string, std::string> Objects::loadStash() {
	std::map<std::string, std::string> files;
	std::ifstream stash(".mank/stash");
	if (!stash.is_open()) return files;

	std::string hash, path;
	while (stash >> hash >> path)
		files[path] = hash;

	return files;
}

std::vector<std::string> Objects::getHistory(const std::string& branchRef) {
	std::vector<std::string> history;
	std::ifstream refFile(branchRef);
	if (!refFile.is_open()) return history;

	std::string current;
	std::getline(refFile, current);

	while (!current.empty()) {
		history.push_back(current);
		std::string content = Objects::read(current);
		if (content.empty()) break;

		std::string parent;
		std::istringstream ss(content);
		std::string line;
		while (std::getline(ss, line))
			if (line.substr(0, 7) == "parent ")
				parent = line.substr(7);

		current = parent;
	}
	return history;
}

std::string Objects::commonAncestor(const std::string& branchA, const std::string& branchB) {
	auto historyA = getHistory(branchA);
	std::set<std::string> setA(historyA.begin(), historyA.end());

	auto historyB = getHistory(branchB);
	for (const auto& commit : historyB)
		if (setA.count(commit))
			return commit;

	return "";
}

static fs::path globalConfigPath() {
    const char* home = std::getenv("HOME");
    return fs::path(home ? home : "~") / ".mankconfig";
}

static std::string readConfigFromFile(const fs::path& path, const std::string& section, const std::string& key) {
    std::ifstream in(path);
    if (!in.is_open()) return "";

    std::string currentSection, line;
    while (std::getline(in, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line.front() == '[' && line.back() == ']') {
            currentSection = line.substr(1, line.size() - 2);
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        k.erase(k.find_last_not_of(" \t") + 1);
        v.erase(0, v.find_first_not_of(" \t"));

        if (currentSection == section && k == key)
            return v;
    }
    return "";
}

static void writeConfigToFile(const fs::path& path, const std::string& section, const std::string& key, const std::string& value) {
    std::vector<std::string> lines;
    std::ifstream in(path);
    std::string line;
    while (std::getline(in, line))
        lines.push_back(line);
    in.close();

    std::string sectionHeader = "[" + section + "]";
    std::string entry = "\t" + key + " = " + value;

    int sectionIdx = -1;
    int keyIdx = -1;

    for (int i = 0; i < (int)lines.size(); i++) {
        std::string trimmed = lines[i];
        size_t start = trimmed.find_first_not_of(" \t");
        if (start != std::string::npos) trimmed = trimmed.substr(start);

        if (trimmed == sectionHeader) { sectionIdx = i; continue; }
        if (sectionIdx != -1 && !trimmed.empty() && trimmed.front() == '[') break;

        if (sectionIdx != -1) {
            auto eq = trimmed.find('=');
            if (eq != std::string::npos) {
                std::string k = trimmed.substr(0, eq);
                k.erase(k.find_last_not_of(" \t") + 1);
                if (k == key) { keyIdx = i; break; }
            }
        }
    }

    if (keyIdx != -1)
        lines[keyIdx] = entry;
    else if (sectionIdx != -1)
        lines.insert(lines.begin() + sectionIdx + 1, entry);
    else {
        lines.push_back("");
        lines.push_back(sectionHeader);
        lines.push_back(entry);
    }

    std::ofstream out(path, std::ios::trunc);
    for (const auto& l : lines)
        out << l << "\n";
}

// Local primero, si no encuentra busca en global
std::string Objects::getConfig(const std::string& section, const std::string& key) {
    std::string val = readConfigFromFile(".mank/config", section, key);
    if (!val.empty()) return val;
    return readConfigFromFile(globalConfigPath(), section, key);
}

// Por defecto escribe en local, con --global en la global
void Objects::setConfig(const std::string& section, const std::string& key, const std::string& value, bool global) {
    fs::path target = global ? globalConfigPath() : fs::path(".mank/config");
    writeConfigToFile(target, section, key, value);
}

bool Objects::isRepo() {
    fs::path current = fs::current_path();

    while (true) {
        if (fs::exists(current / ".mank"))
            return true;

        fs::path parent = current.parent_path();
        if (parent == current) break; // llegamos a la raíz
        current = parent;
    }

    return false;
}

fs::path Objects::getRepoRoot() {
    fs::path current = fs::current_path();

    while (true) {
        if (fs::exists(current / ".mank"))
            return current;

        fs::path parent = current.parent_path();
        if (parent == current) break;
        current = parent;
    }

    return "";
}

void Objects::createTag(const std::string& name, const std::string& commitHash) {
    fs::path tagDir = fs::path(".mank") / "refs" / "tags";
    fs::create_directories(tagDir);

    std::ofstream out(tagDir / name);
    out << commitHash << "\n";
}

std::string Objects::getTag(const std::string& name) {
    fs::path tagFile = fs::path(".mank") / "refs" / "tags" / name;
    if (!fs::exists(tagFile)) return "";

    std::ifstream in(tagFile);
    std::string hash;
    std::getline(in, hash);
    return hash;
}

std::vector<std::pair<std::string, std::string>> Objects::listTags() {
    std::vector<std::pair<std::string, std::string>> tags;
    fs::path tagDir = fs::path(".mank") / "refs" / "tags";

    if (!fs::exists(tagDir)) return tags;

    for (const auto& entry : fs::directory_iterator(tagDir)) {
        std::string name = entry.path().filename().string();
        std::string hash = Objects::getTag(name);
        tags.push_back({name, hash});
    }
    return tags;
}

std::vector<Objects::Submodule> Objects::loadSubmodules() {
    std::vector<Submodule> submodules;
    fs::path path = fs::path(".mank") / "submodules";
    if (!fs::exists(path)) return submodules;

    std::ifstream in(path);
    std::string line;
    Submodule current;

    while (std::getline(in, line)) {
        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;
        line = line.substr(start);

        if (line.front() == '[' && line.back() == ']') {
            if (!current.name.empty())
                submodules.push_back(current);
            current = Submodule();
            current.name = line.substr(1, line.size() - 2);
            continue;
        }

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string k = line.substr(0, eq);
        std::string v = line.substr(eq + 1);
        k.erase(k.find_last_not_of(" \t") + 1);
        v.erase(0, v.find_first_not_of(" \t"));

        if (k == "path")   current.path   = v;
        if (k == "repo")   current.repo   = v;
        if (k == "commit") current.commit = v;
    }

    if (!current.name.empty())
        submodules.push_back(current);

    return submodules;
}

void Objects::saveSubmodules(const std::vector<Objects::Submodule>& submodules) {
    std::ofstream out(fs::path(".mank") / "submodules", std::ios::trunc);
    for (const auto& sub : submodules) {
        out << "[" << sub.name << "]\n";
        out << "\tpath   = " << sub.path   << "\n";
        out << "\trepo   = " << sub.repo   << "\n";
        out << "\tcommit = " << sub.commit << "\n\n";
    }
}
