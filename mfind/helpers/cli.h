#pragma once
#include <string>
#include <stdexcept>
#include <vector>
#include <set>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <filesystem>
struct KeywordRule { std::string id, tag, pattern; };
struct Options {
    std::string root = ".", keywords = "keywords.txt", color = "auto", format = "text", matchMode = "all";
    bool watch = false, help = false, version = false, ignoreCase = false, wholeWord = false, stats = false;
    size_t threads = 4, pollMs = 1000;
    std::vector<std::string> extensions, requireAll, requireAny;
    std::string config, profile, cache;
    size_t indexBytes = 64 * 1024 * 1024;
    std::vector<KeywordRule> rules;
    std::set<std::string> explicitOptions;
};
inline size_t positiveNumber(const std::string& value, size_t maximum) {
    if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) throw std::runtime_error("Expected a positive integer: " + value);
    size_t result = std::stoull(value);
    if (result == 0 || result > maximum) throw std::runtime_error("Integer out of range: " + value);
    return result;
}
inline void applyProfile(Options& options);
inline Options parseOptions(int argc, char** argv) {
    Options options; bool rootSeen = false, positional = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (!positional && arg.rfind("--", 0) == 0) options.explicitOptions.insert(arg);
        auto value = [&]() -> std::string { if (++i >= argc) throw std::runtime_error("Missing value for " + arg); return argv[i]; };
        if (!positional && arg == "--") positional = true;
        else if (!positional && arg == "--watch") options.watch = true;
        else if (!positional && arg == "--ignore-case") options.ignoreCase = true;
        else if (!positional && arg == "--whole-word") options.wholeWord = true;
        else if (!positional && arg == "--help") options.help = true;
        else if (!positional && arg == "--version") options.version = true;
        else if (!positional && arg == "--cache") options.cache = value();
        else if (!positional && arg == "--index-mb") options.indexBytes = positiveNumber(value(), 1024) * 1024 * 1024;
        else if (!positional && arg == "--stats") options.stats = true;
        else if (!positional && arg == "--require-all") options.requireAll.push_back(value());
        else if (!positional && arg == "--require-any") options.requireAny.push_back(value());
        else if (!positional && arg == "--config") options.config = value();
        else if (!positional && arg == "--profile") options.profile = value();
        else if (!positional && arg == "--keywords") options.keywords = value();
        else if (!positional && arg == "--threads") options.threads = positiveNumber(value(), 256);
        else if (!positional && arg == "--poll-ms") options.pollMs = positiveNumber(value(), 3600000);
        else if (!positional && arg == "--match-mode") options.matchMode = value();
        else if (!positional && arg == "--format") options.format = value();
        else if (!positional && arg == "--color") options.color = value();
        else if (!positional && arg == "--extension") { auto ext = value(); if (ext.empty()) throw std::runtime_error("Empty extension"); if (ext[0] != '.') ext = '.' + ext; options.extensions.push_back(ext); }
        else if (!positional && arg.rfind("-", 0) == 0) throw std::runtime_error("Unknown option: " + arg);
        else { if (rootSeen) throw std::runtime_error("Only one root directory is supported"); options.root = arg; rootSeen = true; options.explicitOptions.insert("root"); }
    }
    if (!options.requireAll.empty() && !options.requireAny.empty()) throw std::runtime_error("Choose --require-all or --require-any");
    applyProfile(options);
    if (options.color != "auto" && options.color != "always" && options.color != "never") throw std::runtime_error("Color must be auto, always, or never");
    if (options.format != "text" && options.format != "editor" && options.format != "json" && options.format != "jsonl" && options.format != "csv") throw std::runtime_error("Unknown output format: " + options.format);
    if (options.watch && (options.format == "json" || options.format == "csv")) throw std::runtime_error("Use text, editor, or jsonl for watch output");
    if (options.matchMode != "all" && options.matchMode != "longest-per-start" && options.matchMode != "leftmost-longest") throw std::runtime_error("Unknown match mode: " + options.matchMode);
    return options;
}
inline void applyProfile(Options& options) {
    if (options.config.empty() && options.profile.empty()) return;
    if (options.config.empty() || options.profile.empty()) throw std::runtime_error("Use --config and --profile together");
    std::ifstream input(options.config);
    if (!input) throw std::runtime_error("Cannot read profile file: " + options.config);
    std::string line, current; bool version = false, found = false;
    std::set<std::string> sections, ids;
    while (std::getline(input, line)) {
        std::istringstream row(line); std::string key; if (!(row >> key) || key[0] == '#') continue;
        auto value = [&]() { std::string value; if (!(row >> std::quoted(value))) throw std::runtime_error("Missing profile value: " + key); return value; };
        if (key == "version") { if (version || value() != "1" || !current.empty()) throw std::runtime_error("Expected profile version 1 once at the top"); version = true; }
        else if (key == "profile") {
            if (!version) throw std::runtime_error("Profile file must start with version 1");
            current = value(); if (!sections.insert(current).second) throw std::runtime_error("Duplicate profile: " + current);
            ids.clear(); if (current == options.profile) found = true;
        } else {
            if (current.empty()) throw std::runtime_error("Setting outside profile: " + key);
            const bool selected = current == options.profile;
            if (key == "keyword") {
                KeywordRule rule{value(), value(), value()};
                if (rule.id.empty() || rule.pattern.empty() || !ids.insert(rule.id).second) throw std::runtime_error("Empty/duplicate keyword identity or empty pattern");
                if (selected && !options.explicitOptions.count("--keywords")) options.rules.push_back(rule);
            } else if (key == "root" || key == "extension" || key == "match-mode" || key == "ignore-case" || key == "whole-word") {
                const auto setting = value();
                if ((key == "ignore-case" || key == "whole-word") && setting != "true" && setting != "false") throw std::runtime_error("Profile boolean must be true or false");
                if (selected && !options.explicitOptions.count(key == "root" ? "root" : "--" + key)) {
                    if (key == "root") options.root = (std::filesystem::absolute(options.config).parent_path() / setting).lexically_normal().string();
                    else if (key == "extension") options.extensions.push_back(setting.empty() || setting[0] == '.' ? setting : '.' + setting);
                    else if (key == "match-mode") options.matchMode = setting;
                    else if (key == "ignore-case") options.ignoreCase = setting == "true";
                    else options.wholeWord = setting == "true";
                }
            } else throw std::runtime_error("Unknown profile setting: " + key);
        }
        std::string extra; if (row >> extra) throw std::runtime_error("Unexpected profile field: " + extra);
    }
    if (!found) throw std::runtime_error("Unknown profile: " + options.profile);
    if (!options.explicitOptions.count("--keywords") && options.rules.empty()) throw std::runtime_error("Profile has no keywords");
}
