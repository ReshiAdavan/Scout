#include "ignoreRules.h"
#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace {
std::string globRegex(const std::string& glob) {
    std::string result;
    for (size_t i = 0; i < glob.size(); ++i) {
        const char ch = glob[i];
        if (ch == '*') {
            if (i + 1 < glob.size() && glob[i + 1] == '*') {
                ++i;
                if (i + 1 < glob.size() && glob[i + 1] == '/') { ++i; result += "(?:.*/)?"; }
                else result += ".*";
            } else result += "[^/]*";
        } else if (ch == '?') result += "[^/]";
        else if (ch == '[') {
            auto end = glob.find(']', i + 1);
            if (end == std::string::npos) { result += "\\["; continue; }
            result += '[';
            size_t j = i + 1;
            if (glob[j] == '!') { result += '^'; ++j; }
            result += glob.substr(j, end - j) + ']'; i = end;
        } else {
            char literal = ch;
            if (ch == '\\' && i + 1 < glob.size()) literal = glob[++i];
            if (std::string(".^$|()[]{}+*?\\").find(literal) != std::string::npos) result += '\\';
            result += literal;
        }
    }
    return result;
}
void walk(const std::filesystem::path& dir, IgnoreRules rules, const IgnoreRules& overrides,
          std::vector<std::string>& files) {
    rules.loadFromFile((dir / ".gitignore").string());
    for (const auto& entry : std::filesystem::directory_iterator(dir)) {
        if (entry.is_symlink()) continue;
        const auto name = entry.path().filename().string();
        if (name == ".git" || name == ".cache" || name == ".match_index") continue;
        if (rules.shouldIgnore(entry.path()) || overrides.shouldIgnore(entry.path())) continue;
        if (entry.is_directory()) walk(entry.path(), rules, overrides, files);
        else if (entry.is_regular_file()) files.push_back(entry.path().string());
    }
}
}
void IgnoreRules::loadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) return;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        while (!line.empty() && line.back() == ' ' && (line.size() < 2 || line[line.size()-2] != '\\')) line.pop_back();
        if (line.empty() || line.front() == '#') continue;
        const bool negate = line.front() == '!';
        if (negate) line.erase(0, 1);
        const bool directory = !line.empty() && line.back() == '/';
        if (directory) line.pop_back();
        bool anchored = !line.empty() && line.front() == '/';
        if (anchored) line.erase(0, 1);
        anchored = anchored || line.find('/') != std::string::npos;
        if (line.empty()) continue;
        const std::string regex = (anchored ? "^" : "(?:^|.*/)") + globRegex(line) + "$";
        try {
            rules.push_back({std::filesystem::absolute(path).parent_path().lexically_normal(),
                             std::regex(regex), negate, directory});
        } catch (const std::regex_error&) {
            throw std::runtime_error("Invalid ignore pattern in " + path + ": " + line);
        }
    }
}
bool IgnoreRules::shouldIgnore(const std::filesystem::path& path) const {
    const auto absolute = std::filesystem::absolute(path).lexically_normal();
    bool ignored = false;
    for (const auto& rule : rules) {
        const auto relative = absolute.lexically_relative(rule.base);
        if (relative.empty() || *relative.begin() == "..") continue;
        std::string prefix;
        for (auto it = relative.begin(); it != relative.end(); ++it) {
            if (!prefix.empty()) prefix += '/';
            prefix += it->string();
            const bool ancestor = std::next(it) != relative.end();
            const bool isDirectory = ancestor || std::filesystem::is_directory(absolute);
            if ((!rule.directory || isDirectory) && std::regex_match(prefix, rule.pattern)) ignored = !rule.negate;
        }
    }
    return ignored;
}
std::vector<std::string> collectFiles(const std::filesystem::path& root, const IgnoreRules& overrides) {
    if (!std::filesystem::is_directory(root)) throw std::runtime_error("Not a directory: " + root.string());
    std::vector<std::string> files;
    walk(root, IgnoreRules(), overrides, files);
    std::sort(files.begin(), files.end());
    return files;
}
