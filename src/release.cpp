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
#include "decorations.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <ctime>
#include <zip.h>
#include <set>

namespace fs = std::filesystem;

// Añadir un archivo al zip
static void zipAddFile(zip_t* archive, const fs::path& filePath, const std::string& zipPath) {
    zip_source_t* source = zip_source_file(archive, filePath.string().c_str(), 0, -1);
    if (source)
        zip_file_add(archive, zipPath.c_str(), source, ZIP_FL_OVERWRITE);
}

// Añadir recursivamente una carpeta al zip
static void zipAddDir(zip_t* archive, const fs::path& dir, const std::string& prefix) {
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!entry.is_regular_file()) continue;
        std::string rel = fs::relative(entry.path(), dir).string();
        zipAddFile(archive, entry.path(), prefix + "/" + rel);
    }
}

// Obtener colaboradores únicos del historial
static std::vector<std::string> getContributors() {
    std::vector<std::string> contributors;
    std::string ref = Objects::getCurrentRef();
    auto history = Objects::getHistory(ref);

    std::set<std::string> seen;
    for (const auto& hash : history) {
        std::string content = Objects::read(hash);
        std::istringstream ss(content);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.substr(0, 3) == "by ") {
                std::string name = line.substr(3);
                if (!seen.count(name)) {
                    seen.insert(name);
                    contributors.push_back(name);
                }
            }
        }
    }
    return contributors;
}

// Obtener commits desde el tag anterior hasta el actual
static std::vector<std::pair<std::string, std::string>> getCommitsSinceLastTag(const std::string& currentTagHash) {
    std::vector<std::pair<std::string, std::string>> commits; // hash, message

    // Buscar tags existentes para encontrar el anterior
    fs::path tagDir = fs::path(".mank") / "refs" / "tags";
    std::string prevTagHash;
    if (fs::exists(tagDir)) {
        for (const auto& entry : fs::directory_iterator(tagDir)) {
            std::string h = Objects::getTag(entry.path().filename().string());
            if (h != currentTagHash && !h.empty()) {
                prevTagHash = h;
                break;
            }
        }
    }

    auto history = Objects::getHistory(Objects::getCurrentRef());
    for (const auto& hash : history) {
        if (hash == prevTagHash) break;

        std::string content = Objects::read(hash);
        std::string message;
        std::istringstream ss(content);
        std::string line;
        while (std::getline(ss, line)) {
            if (line.empty()) {
                std::getline(ss, message);
                break;
            }
        }
        commits.push_back({hash.substr(0, 8), message});
    }
    return commits;
}

// Generar changelog manual abriendo el editor
static std::string getManualChangelog() {
    fs::path tmp = fs::path(".mank") / "CHANGELOG_EDIT";
    std::ofstream out(tmp);
    out << "# Write your changelog here\n";
    out << "# Lines starting with # are ignored\n\n";
    out.close();

    const char* ed = std::getenv("EDITOR");
    std::string cmd = std::string(ed ? ed : "nano") + " " + tmp.string();
    system(cmd.c_str());

    std::ifstream in(tmp);
    std::ostringstream ss;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line[0] == '#') continue;
        ss << line << "\n";
    }
    fs::remove(tmp);
    return ss.str();
}

int Mank::release(const std::string& tagName) {
    // Verificar que la tag existe
    std::string commitHash = Objects::getTag(tagName);
    if (commitHash.empty()) {
        Log::error("Tag not found: " + tagName);
        Log::info("Create it first with: mank tag " + tagName);
        return 1;
    }

    // Obtener el tree del commit
    std::string content = Objects::read(commitHash);
    std::string treeHash;
    std::istringstream cs(content);
    std::string line;
    while (std::getline(cs, line))
        if (line.substr(0, 5) == "tree ")
            treeHash = line.substr(5);

    if (treeHash.empty()) {
        Log::error("Could not read tree from commit.");
        return 1;
    }

    // Preguntar changelog
    std::string changelog;
    std::cout << ansi::BOLD << "Changelog: (a)uto / (m)anual? " << ansi::RESET;
    char choice;
    std::cin >> choice;
    std::cin.ignore();

    auto commits = getCommitsSinceLastTag(commitHash);

    if (choice == 'a' || choice == 'A') {
        std::ostringstream cl;
        for (const auto& [hash, msg] : commits)
            cl << "- " << hash << " " << msg << "\n";
        changelog = cl.str();
    } else {
        changelog = getManualChangelog();
    }

    // Crear carpeta de release
    fs::path releaseDir = fs::path(".mank") / "releases" / tagName;
    fs::create_directories(releaseDir);

    // ── 1. ZIP del source ──────────────────────────────────────
    fs::path zipPath = releaseDir / (tagName + "-source.zip");
    int err;
    zip_t* archive = zip_open(zipPath.string().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    if (!archive) {
        Log::error("Could not create zip file.");
        return 1;
    }

    auto tree = Objects::readTree(treeHash);
    for (const auto& [path, hash] : tree) {
        std::string fileContent = Objects::read(hash);
        zip_source_t* src = zip_source_buffer(archive, fileContent.c_str(), fileContent.size(), 0);
        if (src)
            zip_file_add(archive, path.c_str(), src, ZIP_FL_OVERWRITE);
    }
    zip_close(archive);

    // ── 2. TXT ────────────────────────────────────────────────
    auto contributors = getContributors();
    std::time_t now = std::time(nullptr);

    fs::path txtPath = releaseDir / (tagName + "-release.txt");
    std::ofstream txt(txtPath);
    txt << "Release " << tagName << "\n";
    txt << "Date: " << std::ctime(&now);
    txt << "Commit: " << commitHash << "\n\n";

    txt << "== Contributors ==\n";
    for (const auto& c : contributors)
        txt << "  - " << c << "\n";

    txt << "\n== Commits ==\n";
    for (const auto& [hash, msg] : commits)
        txt << "  " << hash << " " << msg << "\n";

    txt << "\n== Changelog ==\n";
    txt << changelog;
    txt.close();

    // ── 3. HTML ───────────────────────────────────────────────
    fs::path htmlPath = releaseDir / (tagName + "-release.html");
    std::ofstream html(htmlPath);
    html << R"(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Release )" << tagName << R"(</title>
    <style>
        body { font-family: monospace; max-width: 800px; margin: 40px auto; padding: 0 20px; background: #0d1117; color: #e6edf3; }
        h1 { color: #58a6ff; border-bottom: 1px solid #30363d; padding-bottom: 10px; }
        h2 { color: #79c0ff; margin-top: 30px; }
        .commit { font-family: monospace; margin: 5px 0; }
        .hash { color: #f78166; margin-right: 10px; }
        .contributor { color: #56d364; margin: 5px 0; }
        .meta { color: #8b949e; font-size: 0.9em; }
        .changelog { background: #161b22; padding: 15px; border-radius: 6px; border: 1px solid #30363d; white-space: pre-wrap; }
    </style>
</head>
<body>
    <h1>Release )" << tagName << R"(</h1>
    <p class="meta">Date: )" << std::ctime(&now) << R"(</p>
    <p class="meta">Commit: )" << commitHash << R"(</p>

    <h2>Contributors</h2>
)";
    for (const auto& c : contributors)
        html << "    <p class=\"contributor\">⬡ " << c << "</p>\n";

    html << "\n    <h2>Commits</h2>\n";
    for (const auto& [hash, msg] : commits)
        html << "    <p class=\"commit\"><span class=\"hash\">" << hash << "</span>" << msg << "</p>\n";

    html << "\n    <h2>Changelog</h2>\n";
    html << "    <div class=\"changelog\">" << changelog << "</div>\n";
    html << "\n</body>\n</html>\n";
    html.close();

    Log::log("Release created: .mank/releases/" + tagName + "/");
    Log::log("  " + tagName + "-source.zip");
    Log::log("  " + tagName + "-release.txt");
    Log::log("  " + tagName + "-release.html");
    
    Mank::ci({"run"}, "release");
    return 0;
}