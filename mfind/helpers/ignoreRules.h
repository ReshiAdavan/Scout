#ifndef IGNORE_RULES_H
#define IGNORE_RULES_H

#include <string>
#include <vector>
#include <unordered_set>
#include <filesystem>

class IgnoreRules {
private:
    std::unordered_set<std::string> exactNames;
    std::unordered_set<std::string> extensions;
    std::unordered_set<std::string> directories;

public:
    void loadFromFile(const std::string& path = ".scoutignore");
    bool shouldIgnore(const std::filesystem::path& p) const;
};

#endif
