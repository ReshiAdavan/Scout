#pragma once
#include "../core/matchIndex.h"
#include <map>
inline std::vector<Match> selectMatches(std::vector<Match> matches, const std::string& mode,
                                       const std::vector<std::string>& keywords) {
    if (mode == "all") return matches;
    std::map<std::string, size_t> order;
    for (size_t i = 0; i < keywords.size(); ++i) order.emplace(keywords[i], i);
    std::stable_sort(matches.begin(), matches.end(), [&](const Match& a, const Match& b) {
        if (a.file != b.file) return a.file < b.file;
        if (a.byteOffset != b.byteOffset) return a.byteOffset < b.byteOffset;
        if (a.keyword.size() != b.keyword.size()) return a.keyword.size() > b.keyword.size();
        return order.at(a.keyword) < order.at(b.keyword);
    });
    std::vector<Match> selected;
    std::string file; size_t lastStart = 0, end = 0; bool first = true;
    for (const auto& match : matches) {
        if (first || match.file != file || (mode == "longest-per-start" ? match.byteOffset != lastStart : match.byteOffset >= end)) {
            selected.push_back(match); file = match.file; lastStart = match.byteOffset;
            end = match.byteOffset + match.keyword.size(); first = false;
        }
    }
    return selected;
}
