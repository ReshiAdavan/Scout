#pragma once
#include "output.h"
#include "../core/searchSession.h"
#include <map>
#include <set>
inline std::vector<Match> queryFiles(const std::vector<Match>& matches, const Options& options,
                                      const std::vector<std::string>& words) {
    std::set<std::string> known;
    if (options.rules.empty()) known.insert(words.begin(), words.end());
    else for (const auto& rule : options.rules) known.insert(rule.id);
    const auto& required = options.requireAll.empty() ? options.requireAny : options.requireAll;
    for (const auto& id : required) if (!known.count(id)) throw std::runtime_error("Unknown query keyword ID: " + id);
    if (required.empty()) return matches;
    std::map<std::string, std::set<std::string>> found;
    for (const auto& match : matches) found[match.file].insert(match.id.empty() ? match.keyword : match.id);
    std::set<std::string> accepted;
    for (const auto& file : found) {
        bool accept = !options.requireAll.empty();
        for (const auto& id : required) {
            if (!options.requireAll.empty()) accept = accept && file.second.count(id);
            else accept = accept || file.second.count(id);
        }
        if (accept) accepted.insert(file.first);
    }
    std::vector<Match> result;
    for (const auto& match : matches) if (accepted.count(match.file)) result.push_back(match);
    return result;
}
inline void printStatistics(const std::vector<Match>& matches, const Options& options,
                            const std::vector<std::string>& words, const SearchSession& session, double elapsedMs) {
    std::map<std::string, size_t> files, keywords;
    std::map<std::string, std::set<std::string>> keywordFiles;
    if (options.rules.empty()) for (const auto& word : words) keywords[word] = 0;
    else for (const auto& rule : options.rules) keywords[rule.id] = 0;
    for (const auto& match : matches) {
        const auto id = match.id.empty() ? match.keyword : match.id;
        ++files[match.file]; ++keywords[id]; keywordFiles[id].insert(match.file);
    }
    std::cerr << "{\"statistics\":{\"occurrences\":" << matches.size() << ",\"matchingFiles\":" << files.size()
              << ",\"scannedFiles\":" << session.scannedFiles << ",\"cacheHits\":" << session.cacheHits
              << ",\"retainedIndexBytes\":" << session.retainedBytes << ",\"evictedFiles\":" << session.evictedFiles
              << ",\"scannedBytes\":" << session.scannedBytes << ",\"elapsedMs\":" << elapsedMs << ",\"files\":{";
    bool first = true;
    for (const auto& file : files) { if (!first) std::cerr << ','; first = false; std::cerr << jsonString(file.first) << ':' << file.second; }
    std::cerr << "},\"keywords\":{"; first = true;
    for (const auto& keyword : keywords) { if (!first) std::cerr << ','; first = false;
        std::cerr << jsonString(keyword.first) << ":{\"occurrences\":" << keyword.second << ",\"files\":" << keywordFiles[keyword.first].size() << '}'; }
    std::cerr << "}}}\n";
}
