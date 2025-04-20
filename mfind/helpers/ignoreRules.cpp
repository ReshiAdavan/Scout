#include "ignoreRules.h"
#include <fstream>
#include <algorithm>

void IgnoreRules::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        // Strip comments and whitespace
        auto hashPos = line.find('#');
        if (hashPos != std::string::npos) line = line.substr(0, hashPos);
        line.erase(std::remove_if(line.begin(), line.end(), ::isspace), line.end());

        if (line.empty()) continue;

        if (line.size() > 1 && line.back() == '/') {
            directories.insert(line.substr(0, line.size() - 1));
        } else if (line.size() > 2 && line[0] == '*' && line[1] == '.') {
            extensions.insert(line.substr(1)); // e.g., ".log"
        } else {
            exactNames.insert(line); // e.g., README.md
        }
    }
}

bool IgnoreRules::shouldIgnore(const std::filesystem::path& p) const {
    std::string filename = p.filename().string();
    std::string extension = p.extension().string();
    std::string dirName = p.has_parent_path() ? p.parent_path().filename().string() : "";

    if (exactNames.count(filename)) return true;
    if (!extension.empty() && extensions.count(extension)) return true;
    if (!dirName.empty() && directories.count(dirName)) return true;

    return false;
}
