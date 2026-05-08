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

/*
 * This files contains all the functions of mank, init, commit, get, leave, release, etc...
 * */

#include "mank.hpp"
#include "log.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include "objects.hpp"
#include "decorations.hpp"
#include <algorithm>
#include <unistd.h>
#include <cstdio>
#include <set>
#include "pager.hpp"

namespace fs = std::filesystem;

// This function initializes a repo in a folder
int Mank::init(std::string dir) {
    fs::path root = fs::path(dir) / ".mank";

    if(fs::exists(root)) {
        Log::error("There's already an initialized repository in " + fs::absolute(root).string());
        Log::info("If you haven't initialized a repository in that folder, maybe you should delete the \".mank\" folder inside it.");
        return 1;
    }

    fs::create_directories(root / "objects" / "info");
    fs::create_directories(root / "objects" / "pack");
    fs::create_directories(root / "refs" / "heads");
    fs::create_directories(root / "refs" / "tags");

    std::ofstream(root / "HEAD") << "ref: refs/heads/main\n";

    // Obtener el nombre del repo a partir de la carpeta
    std::string repoName = fs::absolute(fs::path(dir)).filename().string();

    std::ofstream config(root / "config");
    config << "[core]\n"
           << "\trepositoryformatversion = 0\n"
           << "\tfilemode = true\n"
           << "\tbare = false\n"
           << "\tname = " << repoName << "\n";  // <-- añadir esto

    Log::log("Empty repository created in " + fs::absolute(root).string());
    return 0;
}

int Mank::commit(const std::string& message) {
	// 1. Read the index
	std::ifstream indexFile(".mank/index");
	if (!indexFile.is_open()) {
		Log::error("Nothing to commit, index is empty.");
		return 1;
	}

	std::vector<std::pair<std::string,std::string>> entries;
	std::string hash, path;
	while (indexFile >> hash >> path)
		entries.emplace_back(hash, path);

	if (entries.empty()) {
		Log::error("Nothing to commit.");
		return 1;
	}

	// 2. Create tree
	std::string treeHash = Objects::createTree(entries);

	// 3. Read parent from refs/heads/main
	std::string parent;
	std::ifstream refFile(Objects::getCurrentRef());
	if (refFile.is_open())
		std::getline(refFile, parent);

	// 4. Create commit
	std::string commitHash = Objects::createCommit(treeHash, parent, message);

	// 5. Update ref
	std::ofstream refOut(Objects::getCurrentRef());
	refOut << commitHash << "\n";

	// 6. Clean index
	std::ofstream(".mank/index", std::ios::trunc).close();

	Log::log("Commit: " + commitHash.substr(0, 8) + " - " + message);

	Mank::ci({"run"}, "commit");
	return 0;
}

int Mank::history(bool oneline) {
    std::ifstream refFile(Objects::getCurrentRef());
    if (!refFile.is_open()) {
        Log::error("No commits yet.");
        return 1;
    }

    std::string current;
    std::getline(refFile, current);

    Pager::open();

    while (!current.empty()) {
        std::string content = Objects::read(current);
        if (content.empty()) break;

        std::string tree, parent, date, message, user, email;
        std::istringstream ss(content);
        std::string line;

        while (std::getline(ss, line)) {
            if (line.substr(0, 5) == "tree ")        tree = line.substr(5);
            else if (line.substr(0, 7) == "parent ") parent = line.substr(7);
            else if (line.substr(0, 5) == "date ")   date = line.substr(5);
            else if (line.substr(0, 3) == "by ")     user = line.substr(3);
            else if (line.substr(0, 6) == "email ")  email = line.substr(6);
            else if (line.empty()) {
                std::getline(ss, message);
                break;
            }
        }

        if (oneline) {
            // hash corto + mensaje en una sola línea
            std::cout << ansi::FG_CYAN << current.substr(0, 8) << ansi::RESET
                      << " " << message << "\n";
        } else {
            std::cout << ansi::FG_CYAN << "commit: "  << ansi::RESET << current.substr(0,16) << "\n";
            time_t t = std::stol(date);
            std::cout << ansi::FG_CYAN << "date: "    << ansi::RESET << std::ctime(&t);
            std::cout << ansi::FG_CYAN << "by: "      << ansi::RESET << user << "\n";
            std::cout << ansi::FG_CYAN << "email: "   << ansi::RESET << email << "\n";
            std::cout << ansi::FG_CYAN << "message: " << ansi::RESET << message << "\n\n";
            std::cout << ansi::BOLD << "---------------------------" << ansi::RESET << "\n";
        }

        current = parent;
    }

    Pager::close();
    return 0;
}

int Mank::status() {
	// Cargar el tree del último commit
	std::string treeHash = Objects::getLastTree();
	std::map<std::string, std::string> committed;
	if (!treeHash.empty())
		committed = Objects::readTree(treeHash);

	// Cargar el índice actual
	std::map<std::string, std::string> staged = Objects::loadIndex();

	// Archivos staged
	bool anyStagedChanges = false;
	for (const auto& [path, hash] : staged) {
		if (committed.count(path) == 0) {
			if (!anyStagedChanges) { Log::info("Staged changes:"); anyStagedChanges = true; }
			Log::log("  new file:  " + path);
		} else if (committed[path] != hash) {
			if (!anyStagedChanges) { Log::info("Staged changes:"); anyStagedChanges = true; }
			Log::log("  modified:  " + path);
		}
	}

	// Archivos en disco vs índice
	bool anyUnstaged = false;
	for (const auto& [path, stagedHash] : staged) {
		if (!fs::exists(path)) {
			if (!anyUnstaged) { Log::warning("Not staged:"); anyUnstaged = true; }
			Log::log("  deleted:   " + path);
			continue;
		}
		std::ifstream f(path);
		std::ostringstream ss;
		ss << f.rdbuf();
		std::string currentHash = Objects::hash(ss.str());
		if (currentHash != stagedHash) {
			if (!anyUnstaged) { Log::warning("Not staged:"); anyUnstaged = true; }
			Log::log("  modified:  " + path);
		}
	}

	// Archivos en commit pero borrados del disco y del índice
	for (const auto& [path, hash] : committed) {
		if (!staged.count(path) && !fs::exists(path)) {
			if (!anyUnstaged) { Log::warning("Not staged:"); anyUnstaged = true; }
			Log::log("  deleted:   " + path);
		}
	}

	if (!anyStagedChanges && !anyUnstaged)
		Log::log("Nothing to commit, working tree clean.");

	return 0;
}

std::vector<std::string> splitLines(const std::string& content) {
	std::vector<std::string> lines;
	std::istringstream ss(content);
	std::string line;
	while (std::getline(ss, line))
		lines.push_back(line);
	return lines;
}

void printDiff(const std::vector<std::string>& oldLines,
               const std::vector<std::string>& newLines,
               const std::string& filepath,
               const std::string& oldContent, 
               const std::string& newContent) {
	int n = oldLines.size(), m = newLines.size();

	// Construir tabla LCS
	std::vector<std::vector<int>> dp(n + 1, std::vector<int>(m + 1, 0));
	for (int i = 1; i <= n; i++)
		for (int j = 1; j <= m; j++)
			if (oldLines[i-1] == newLines[j-1])
				dp[i][j] = dp[i-1][j-1] + 1;
			else
				dp[i][j] = std::max(dp[i-1][j], dp[i][j-1]);

	// Reconstruir diff completo
	std::vector<std::tuple<char, std::string, int, int>> diff; // tipo, linea, old_line, new_line
	int i = n, j = m;
	while (i > 0 || j > 0) {
		if (i > 0 && j > 0 && oldLines[i-1] == newLines[j-1]) {
			diff.emplace_back(' ', oldLines[i-1], i, j);
			i--; j--;
		} else if (j > 0 && (i == 0 || dp[i][j-1] >= dp[i-1][j])) {
			diff.emplace_back('+', newLines[j-1], i, j);
			j--;
		} else {
			diff.emplace_back('-', oldLines[i-1], i, j);
			i--;
		}
	}
	std::reverse(diff.begin(), diff.end());

	// Encontrar índices de líneas que cambiaron
	const int CONTEXT = 3;
	std::vector<bool> show(diff.size(), false);
	for (int k = 0; k < (int)diff.size(); k++) {
		if (std::get<0>(diff[k]) != ' ') {
			// Marcar contexto alrededor
			for (int c = std::max(0, k - CONTEXT);
					 c <= std::min((int)diff.size() - 1, k + CONTEXT); c++)
				show[c] = true;
		}
	}

	// Imprimir por hunks
	int k = 0;
	while (k < (int)diff.size()) {
		if (!show[k]) { k++; continue; }

		// Inicio del hunk — calcular números de línea
		int oldStart = std::get<2>(diff[k]);
		int newStart = std::get<3>(diff[k]);
		int oldCount = 0, newCount = 0;

		// Contar líneas del hunk
		int end = k;
		while (end < (int)diff.size() && show[end]) {
			if (std::get<0>(diff[end]) != '+') oldCount++;
			if (std::get<0>(diff[end]) != '-') newCount++;
			end++;
		}

		// Cabecera del hunk
		setColor(32, 180, 220);
		std::cout << "@@ -" << oldStart << "," << oldCount
				  << " +" << newStart << "," << newCount << " @@\n";

		int totalAdded = 0, totalRemoved = 0; // Count exact lines changed

		// Líneas del hunk
		while (k < end) {
			char type = std::get<0>(diff[k]);
			const std::string& line = std::get<1>(diff[k]);
			if (type == '+') {
				setColor(32, 180, 32);
				std::cout << "+ " << line << "\n";
				totalAdded++;
			} else if (type == '-') {
				setColor(201, 32, 32);
				std::cout << "- " << line << "\n";
				totalRemoved++;
			} else {
				resetColor();
				std::cout << "  " << line << "\n";
			}
			k++;
		}

		// Show changes in lines
		resetColor();
		std::cout << std::endl << "Changes: ";
		setColor(32, 180, 32);	std::cout << "+" << totalAdded;
		setColor(32, 180, 220);	std::cout << "/";
		setColor(201, 32, 32);	std::cout << "-" << totalRemoved << std::endl << std::endl;


    		if (totalAdded > 0 || totalRemoved > 0) {
        		std::cout << "\n";
        		setColor(32, 180, 220);
        		if (!filepath.empty()) {
            			std::cout << filepath << " | ";
        		}
        		std::cout << totalAdded + totalRemoved << " changes: ";
        
        		if (totalAdded > 0) {
            			setColor(32, 180, 32);
            			std::cout << "+" << totalAdded;
        		}
        		if (totalAdded > 0 && totalRemoved > 0) {
            			resetColor();
            			std::cout << " ";
        		}
		        if (totalRemoved > 0) {
            			setColor(201, 32, 32);
            			std::cout << "-" << totalRemoved;
        		}
        
        		// Tamaño del archivo si se proporciona
        		if (!oldContent.empty() && !newContent.empty()) {
            			resetColor();
            			std::cout << " | " << oldContent.size() << " → " << newContent.size() << " bytes";
        		}
        		resetColor();
        		std::cout << "\n";
		}

		// Separador entre hunks
		if (k < (int)diff.size() && show[k]) {
			resetColor();
			std::cout << "\n";
		}
	}

	resetColor();
}

static int diffAll();

int Mank::diff(const std::string& filepath) {
	if (filepath == ".") {
		return diffAll();
	}
	
	Pager::open();

	// Leer versión del último commit
	std::string committed = Objects::readFile(filepath);

	// Leer versión actual en disco
	if (!fs::exists(filepath)) {
		Log::error("File not found: " + filepath);
		return 1;
	}
	std::ifstream f(filepath);
	std::ostringstream ss;
	ss << f.rdbuf();
	std::string current = ss.str();

	if (committed == current) {
		Log::log("No changes in " + filepath);
		return 0;
	}

	Log::info("--- " + filepath + " (committed)");
	Log::info("+++ " + filepath + " (current)");
	std::cout << "\n";

	printDiff(splitLines(committed), splitLines(current), 
              filepath, committed, current);

	Pager::close();

	return 0;
}

static int diffAll() {
	std::string treeHash = Objects::getLastTree();
	if (treeHash.empty()) {
		Log::log("No commits yet.");
		return 0;
	}

	auto tree = Objects::readTree(treeHash);

	std::vector<std::string> changed;
	for (const auto& [path, hash] : tree) {
		if (!fs::exists(path)) continue;

		std::ifstream f(path);
		std::ostringstream ss;
		ss << f.rdbuf();
		std::string current = ss.str();

		std::string committed = Objects::read(hash);
		if (committed != current)
		changed.push_back(path);
	}

	if (changed.empty()) {
		Log::log("No changes in any tracked file.");
		return 0;
	}

	Pager::open();

	int totalFiles = 0;

	for (const auto& path : changed) {
		std::string treeHash2 = Objects::getLastTree();
		auto tree2 = Objects::readTree(treeHash2);
		std::string committed = Objects::read(tree2[path]);

		std::ifstream f(path);
		std::ostringstream ss;
		ss << f.rdbuf();
		std::string current = ss.str();

		Log::info("--- " + path + " (committed)");
		Log::info("+++ " + path + " (current)");
		std::cout << "\n";
	        printDiff(splitLines(committed), splitLines(current), 
                  path, committed, current);
		totalFiles++;
		std::cout << "\n";
	}

	// Git-like summary
	if (totalFiles > 0) {
        	std::cout << "\n" << totalFiles << " modified files.";
        	std::cout << "\n";
    	}


	Pager::close();
	return 0;
}

int Mank::branch(const std::string& name) {
	if (name.empty()) {
		// Listar branches
		std::string current = Objects::getCurrentBranch();
		for (const auto& entry : fs::directory_iterator(".mank/refs/heads")) {
			std::string b = entry.path().filename().string();
			if (b == current) {
				setColor(32, 180, 32);
				std::cout << "* " << b << "\n";
				resetColor();
			} else {
				std::cout << "  " << b << "\n";
			}
		}
		return 0;
	}

	// Crear branch apuntando al commit actual
	std::string ref = Objects::getCurrentRef();
	std::ifstream currentRef(ref);
	std::string commitHash;
	std::getline(currentRef, commitHash);

	fs::path newRef = fs::path(".mank/refs/heads") / name;
	if (fs::exists(newRef)) {
		Log::error("Branch already exists: " + name);
		return 1;
	}

	std::ofstream(newRef) << commitHash << "\n";
	Log::log("Created branch: " + name);
	return 0;
}

int Mank::switchBranch(const std::string& name) {
	fs::path ref = fs::path(".mank/refs/heads") / name;
	if (!fs::exists(ref)) {
		Log::error("Branch not found: " + name);
		return 1;
	}

	// Leer commit de la branch destino
	std::ifstream refFile(ref);
	std::string commitHash;
	std::getline(refFile, commitHash);

	// Restaurar archivos del tree de ese commit
	if (!commitHash.empty()) {
		std::string content = Objects::read(commitHash);
		std::istringstream ss(content);
		std::string line, treeHash;
		while (std::getline(ss, line))
			if (line.substr(0, 5) == "tree ")
				treeHash = line.substr(5);

		auto tree = Objects::readTree(treeHash);
		for (const auto& [path, hash] : tree) {
			std::string fileContent = Objects::read(hash);
			fs::create_directories(fs::path(path).parent_path());
			std::ofstream out(path);
			out << fileContent;
		}
	}

	// Actualizar HEAD
	std::ofstream head(".mank/HEAD");
	head << "ref: refs/heads/" << name << "\n";

	// Limpiar índice
	std::ofstream(".mank/index", std::ios::trunc).close();

	Log::log("Switched to branch: " + name);
	return 0;
}

int Mank::restore(const std::string& filepath) {
	std::string content = Objects::readFile(filepath);
	if (content.empty()) {
		Log::error("File not found in last commit: " + filepath);
		return 1;
	}

	std::ofstream out(filepath);
	out << content;

	// Actualizar índice
	auto index = Objects::loadIndex();
	std::string h = Objects::hash(content);
	index[filepath] = h;
	std::ofstream indexOut(".mank/index", std::ios::trunc);
	for (const auto& [p, hsh] : index)
		indexOut << hsh << " " << p << "\n";

	Log::log("Restored: " + filepath);
	return 0;
}

int Mank::stash() {
	// Buscar archivos modificados respecto al último commit
	std::string treeHash = Objects::getLastTree();
	std::map<std::string, std::string> committed;
	if (!treeHash.empty())
		committed = Objects::readTree(treeHash);

	std::map<std::string, std::string> toStash;

	// Archivos modificados en disco respecto al commit
	for (const auto& [path, committedHash] : committed) {
		if (!fs::exists(path)) continue;
		std::ifstream f(path);
		std::ostringstream ss;
		ss << f.rdbuf();
		std::string content = ss.str();
		std::string currentHash = Objects::hash(content);

		if (currentHash != committedHash) {
			// Guardar objeto con contenido actual
			Objects::store(currentHash, content);
			toStash[path] = currentHash;
		}
	}

	// Archivos en índice que son nuevos
	auto staged = Objects::loadIndex();
	for (const auto& [path, hash] : staged) {
		if (!committed.count(path))
			toStash[path] = hash;
	}

	if (toStash.empty()) {
		Log::log("Nothing to stash.");
		return 0;
	}

	// Guardar stash
	Objects::saveStash(toStash);

	// Restaurar archivos al estado del último commit
	for (const auto& [path, hash] : toStash) {
		if (committed.count(path)) {
			std::string content = Objects::read(committed[path]);
			std::ofstream out(path);
			out << content;
		} else {
			// Archivo nuevo, borrarlo
			fs::remove(path);
		}
	}

	// Limpiar índice
	std::ofstream(".mank/index", std::ios::trunc).close();

	Log::log("Stashed " + std::to_string(toStash.size()) + " file(s).");
	return 0;
}

int Mank::stashPop() {
	auto stash = Objects::loadStash();
	if (stash.empty()) {
		Log::error("No stash found.");
		return 1;
	}

	// Restaurar archivos del stash
	for (const auto& [path, hash] : stash) {
		std::string content = Objects::read(hash);
		fs::path parent = fs::path(path).parent_path();
		if (!parent.empty())
			fs::create_directories(parent);
		std::ofstream out(path);
		out << content;

		// Actualizar índice
		auto index = Objects::loadIndex();
		index[path] = hash;
		std::ofstream indexOut(".mank/index", std::ios::trunc);
		for (const auto& [p, hsh] : index)
			indexOut << hsh << " " << p << "\n";
	}

	// Borrar stash
	fs::remove(".mank/stash");

	Log::log("Popped stash: " + std::to_string(stash.size()) + " file(s) restored.");
	return 0;
}

enum class MergeResult { Clean, Conflict };

static MergeResult mergeFile(const std::string& base,
							  const std::string& ours,
							  const std::string& theirs,
							  const std::string& ourBranch,
							  const std::string& theirBranch,
							  std::string& output) {
	auto baseLines  = splitLines(base);
	auto oursLines  = splitLines(ours);
	auto theirsLines = splitLines(theirs);

	// Si uno de los dos no cambió respecto al ancestro, tomar el otro
	if (ours == base)   { output = theirs; return MergeResult::Clean; }
	if (theirs == base) { output = ours;   return MergeResult::Clean; }
	if (ours == theirs) { output = ours;   return MergeResult::Clean; }

	// Ambos cambiaron — merge línea a línea con LCS
	// Primero diff base->ours, luego base->theirs
	auto diffWith = [](const std::vector<std::string>& from,
					   const std::vector<std::string>& to)
		-> std::vector<std::pair<char, std::string>> {

		int n = from.size(), m = to.size();
		std::vector<std::vector<int>> dp(n+1, std::vector<int>(m+1, 0));
		for (int i = 1; i <= n; i++)
			for (int j = 1; j <= m; j++)
				dp[i][j] = from[i-1] == to[j-1]
					? dp[i-1][j-1] + 1
					: std::max(dp[i-1][j], dp[i][j-1]);

		std::vector<std::pair<char,std::string>> result;
		int i = n, j = m;
		while (i > 0 || j > 0) {
			if (i > 0 && j > 0 && from[i-1] == to[j-1]) {
				result.emplace_back(' ', from[i-1]); i--; j--;
			} else if (j > 0 && (i == 0 || dp[i][j-1] >= dp[i-1][j])) {
				result.emplace_back('+', to[j-1]); j--;
			} else {
				result.emplace_back('-', from[i-1]); i--;
			}
		}
		std::reverse(result.begin(), result.end());
		return result;
	};

	auto oursChanges   = diffWith(baseLines, oursLines);
	auto theirsChanges = diffWith(baseLines, theirsLines);

	// Intentar combinar los dos diffs
	// Si las líneas modificadas no se solapan → merge limpio
	// Si se solapan → conflicto
	std::ostringstream out;
	bool hasConflict = false;

	size_t oi = 0, ti = 0;
	while (oi < oursChanges.size() || ti < theirsChanges.size()) {
		bool oursChanged   = oi < oursChanges.size()   && oursChanges[oi].first   != ' ';
		bool theirsChanged = ti < theirsChanges.size() && theirsChanges[ti].first != ' ';

		if (!oursChanged && !theirsChanged) {
			// Ambos tienen la misma línea sin cambios
			out << oursChanges[oi].second << "\n";
			oi++; ti++;
		} else if (oursChanged && !theirsChanged) {
			if (oursChanges[oi].first == '+') out << oursChanges[oi].second << "\n";
			oi++;
		} else if (!oursChanged && theirsChanged) {
			if (theirsChanges[ti].first == '+') out << theirsChanges[ti].second << "\n";
			ti++;
		} else {
			// Conflicto — ambos modificaron esta zona
			hasConflict = true;
			out << "<<<<<<< yours (" << ourBranch << ")\n";
			while (oi < oursChanges.size() && oursChanges[oi].first != ' ') {
				if (oursChanges[oi].first == '+') out << oursChanges[oi].second << "\n";
				oi++;
			}
			out << "===========\n";
			while (ti < theirsChanges.size() && theirsChanges[ti].first != ' ') {
				if (theirsChanges[ti].first == '+') out << theirsChanges[ti].second << "\n";
				ti++;
			}
			out << ">>>>>>> theirs (" << theirBranch << ")\n";
		}
	}

	output = out.str();
	return hasConflict ? MergeResult::Conflict : MergeResult::Clean;
}

int Mank::merge(const std::string& branchName) {
	std::string currentBranch = Objects::getCurrentBranch();
	std::string currentRef    = Objects::getCurrentRef();
	std::string theirRef      = ".mank/refs/heads/" + branchName;

	if (!fs::exists(theirRef)) {
		Log::error("Branch not found: " + branchName);
		return 1;
	}

	// Encontrar ancestro común
	std::string ancestor = Objects::commonAncestor(currentRef, theirRef);

	// Leer trees
	auto getTree = [](const std::string& ref) -> std::map<std::string,std::string> {
		std::ifstream f(ref);
		std::string commitHash;
		std::getline(f, commitHash);
		if (commitHash.empty()) return {};
		std::string content = Objects::read(commitHash);
		std::istringstream ss(content);
		std::string line, treeHash;
		while (std::getline(ss, line))
			if (line.substr(0, 5) == "tree ") treeHash = line.substr(5);
		return Objects::readTree(treeHash);
	};

	auto baseTree   = ancestor.empty() ? std::map<std::string,std::string>{} 
									   : Objects::readTree([&](){
							std::string c = Objects::read(ancestor);
							std::istringstream ss(c); std::string l, t;
							while(std::getline(ss,l)) if(l.substr(0,5)=="tree ") t=l.substr(5);
							return t;
						}());
	auto oursTree   = getTree(currentRef);
	auto theirsTree = getTree(theirRef);

	// Unión de todos los paths
	std::set<std::string> allPaths;
	for (const auto& [p,_] : oursTree)   allPaths.insert(p);
	for (const auto& [p,_] : theirsTree) allPaths.insert(p);

	int conflicts = 0;
	std::cout << "Merging " << branchName << " -> " << currentBranch << "\n";
	std::cout << "------------------------------------\n";

	for (const auto& path : allPaths) {
		std::string base   = baseTree.count(path)   ? Objects::read(baseTree.at(path))   : "";
		std::string ours   = oursTree.count(path)   ? Objects::read(oursTree.at(path))   : "";
		std::string theirs = theirsTree.count(path) ? Objects::read(theirsTree.at(path)) : "";

		std::string output;
		auto result = mergeFile(base, ours, theirs, currentBranch, branchName, output);

		if (result == MergeResult::Clean) {
			setColor(32, 180, 32);
			std::cout << "  ok        ";
			resetColor();
			std::cout << " " << path << "\n";
		} else {
			setColor(201, 32, 32);
			std::cout << "  conflict  ";
			resetColor();
			std::cout << " " << path << "\n";
			conflicts++;
		}

		// Escribir resultado en disco
		fs::path parent = fs::path(path).parent_path();
		if (!parent.empty()) fs::create_directories(parent);
		std::ofstream out(path);
		out << output;

		// Actualizar índice
		auto index = Objects::loadIndex();
		std::string h = Objects::hash(output);
		Objects::store(h, output);
		index[path] = h;
		std::ofstream indexOut(".mank/index", std::ios::trunc);
		for (const auto& [p, hsh] : index)
			indexOut << hsh << " " << p << "\n";
	}

	std::cout << "------------------------------------\n";
	if (conflicts == 0) {
		Log::log("Merge clean. You can now run:");
		Log::info("  mank -c \"merge: " + branchName + "\"");
	} else {
		Log::warning(std::to_string(conflicts) + " conflict(s) found. Resolve them and run:");
		Log::info("  mank -c \"merge: " + branchName + "\"");
	}

	Mank::ci({"run"}, "merge");
	return conflicts > 0 ? 1 : 0;
}

int Mank::config(const std::string& section, const std::string& key, const std::string& value, bool global) {
    if (section == "user" && key != "name" && key != "email") {
        Log::error("Unknown config key: " + key);
        Log::info(global ? "Valid keys: --config.name, --config.email" : "Valid keys: --gconfig.name, --gconfig.email");
        return 1;
    }

    Objects::setConfig(section, key, value, global);
    Log::log("Config updated: [" + section + "] " + key + " = " + value);
    return 0;
}

int Mank::unstage(const std::string& filepath) {
    auto index = Objects::loadIndex();

    if (filepath == ".") {
        if (index.empty()) {
            Log::log("Nothing to unstage.");
            return 0;
        }
        index.clear();
        Log::log("Unstaged all files.");
    } else {
        std::string normalized = fs::path(filepath).lexically_normal().string();
        if (!index.count(normalized)) {
            Log::error("File not in index: " + normalized);
            return 1;
        }
        index.erase(normalized);
        Log::log("Unstaged: " + normalized);
    }

    // Reescribir el índice
    std::ofstream indexOut(".mank/index", std::ios::trunc);
    for (const auto& [p, h] : index)
        indexOut << h << " " << p << "\n";

    return 0;
}

int Mank::tag(const std::string& name) {
    // Sin argumento, listar todas las tags
    if (name.empty()) {
        auto tags = Objects::listTags();
        if (tags.empty()) {
            Log::log("No tags yet.");
            return 0;
        }
        for (const auto& [n, hash] : tags)
            Log::log(n + " -> " + hash.substr(0, 8) + "...");
        return 0;
    }

    // Comprobar que no existe ya
    if (!Objects::getTag(name).empty()) {
        Log::error("Tag already exists: " + name);
        return 1;
    }

    // Obtener el commit actual
    std::ifstream refFile(Objects::getCurrentRef());
    if (!refFile.is_open()) {
        Log::error("No commits yet.");
        return 1;
    }
    std::string commitHash;
    std::getline(refFile, commitHash);

    if (commitHash.empty()) {
        Log::error("No commits yet.");
        return 1;
    }

    Objects::createTag(name, commitHash);
    Log::log("Tag created: " + name + " -> " + commitHash.substr(0, 8) + "...");
    return 0;
}

int Mank::diffCommits(const std::string& hashA, const std::string& hashB) {
    std::string contentA = Objects::read(hashA);
    std::string contentB = Objects::read(hashB);

    if (contentA.empty()) { Log::error("Commit not found: " + hashA); return 1; }
    if (contentB.empty()) { Log::error("Commit not found: " + hashB); return 1; }

    // Extraer el tree de cada commit
    auto getTree = [](const std::string& content) {
        std::istringstream ss(content);
        std::string line;
        while (std::getline(ss, line))
            if (line.substr(0, 5) == "tree ")
                return line.substr(5);
        return std::string("");
    };

    std::string treeA = getTree(contentA);
    std::string treeB = getTree(contentB);

    if (treeA.empty() || treeB.empty()) {
        Log::error("Could not read trees from commits.");
        return 1;
    }

    auto filesA = Objects::readTree(treeA);
    auto filesB = Objects::readTree(treeB);

    Pager::open();

    // Modificados y añadidos
    for (const auto& [path, hashB_] : filesB) {
        std::string contentNew = Objects::read(hashB_);
        std::string contentOld;

        if (filesA.count(path))
            contentOld = Objects::read(filesA.at(path));

        if (contentOld == contentNew) continue;

        Log::info("--- " + path + " (" + hashA.substr(0, 8) + ")");
        Log::info("+++ " + path + " (" + hashB.substr(0, 8) + ")");
        std::cout << "\n";
        printDiff(splitLines(contentOld), splitLines(contentNew),
		path, contentOld, contentNew);
        std::cout << "\n";
    }

    // Eliminados
    for (const auto& [path, _] : filesA) {
        if (!filesB.count(path))
            std::cout << ansi::BOLD << ansi::FG_RED << "deleted: " << path << ansi::RESET << "\n";
    }

    Pager::close();
    return 0;
}
