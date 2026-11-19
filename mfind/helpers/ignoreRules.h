#ifndef IGNORE_RULES_H
#define IGNORE_RULES_H
#include <filesystem>
#include <regex>
#include <string>
#include <vector>
class IgnoreRules {
    struct Rule {
        std::filesystem::path base;
        std::regex pattern;
        bool negate;
        bool directory;
    };
    std::vector<Rule> rules;
public:
    void loadFromFile(const std::string& path = ".scoutignore");
    bool shouldIgnore(const std::filesystem::path& path) const;
};
// Traversal copies rules into each subtree, so nested .gitignore rules remain scoped.
std::vector<std::string> collectFiles(const std::filesystem::path& root,
                                     const IgnoreRules& overrides = IgnoreRules());
#endif
