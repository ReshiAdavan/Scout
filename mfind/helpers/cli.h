#pragma once
#include <string>
#include <stdexcept>
#include <vector>
struct Options {
    std::string root = ".", keywords = "keywords.txt", color = "auto";
    bool watch = false, help = false, version = false;
    size_t threads = 4, pollMs = 1000;
    std::vector<std::string> extensions;
};
inline size_t positiveNumber(const std::string& value, size_t maximum) {
    if (value.empty() || value.find_first_not_of("0123456789") != std::string::npos) throw std::runtime_error("Expected a positive integer: " + value);
    size_t result = std::stoull(value);
    if (result == 0 || result > maximum) throw std::runtime_error("Integer out of range: " + value);
    return result;
}
inline Options parseOptions(int argc, char** argv) {
    Options options; bool rootSeen = false, positional = false;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto value = [&]() -> std::string { if (++i >= argc) throw std::runtime_error("Missing value for " + arg); return argv[i]; };
        if (!positional && arg == "--") positional = true;
        else if (!positional && arg == "--watch") options.watch = true;
        else if (!positional && arg == "--help") options.help = true;
        else if (!positional && arg == "--version") options.version = true;
        else if (!positional && arg == "--keywords") options.keywords = value();
        else if (!positional && arg == "--threads") options.threads = positiveNumber(value(), 256);
        else if (!positional && arg == "--poll-ms") options.pollMs = positiveNumber(value(), 3600000);
        else if (!positional && arg == "--color") options.color = value();
        else if (!positional && arg == "--extension") { auto ext = value(); if (ext.empty()) throw std::runtime_error("Empty extension"); if (ext[0] != '.') ext = '.' + ext; options.extensions.push_back(ext); }
        else if (!positional && arg.rfind("-", 0) == 0) throw std::runtime_error("Unknown option: " + arg);
        else { if (rootSeen) throw std::runtime_error("Only one root directory is supported"); options.root = arg; rootSeen = true; }
    }
    if (options.color != "auto" && options.color != "always" && options.color != "never") throw std::runtime_error("Color must be auto, always, or never");
    return options;
}
