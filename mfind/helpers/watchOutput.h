#pragma once
#include "output.h"
#include <map>
inline void printChanges(const std::vector<Match>& previous, const std::vector<Match>& current,
                         const Options& options) {
    // Full exported value is the identity: changed labels/context produce remove/add.
    std::map<std::string, Match> before, after;
    for (const auto& match : previous) before.emplace(matchJson(match), match);
    for (const auto& match : current) after.emplace(matchJson(match), match);
    auto emit = [&](const char* action, const std::pair<const std::string, Match>& entry) {
        if (options.format == "jsonl") std::cout << "{\"event\":" << jsonString(action) << ",\"match\":" << entry.first << "}\n";
        else { std::cout << (std::string(action) == "added" ? "+ " : "- "); printResults({entry.second}, options); }
    };
    for (const auto& entry : before) if (!after.count(entry.first)) emit("removed", entry);
    for (const auto& entry : after) if (!before.count(entry.first)) emit("added", entry);
    std::cout.flush();
}
