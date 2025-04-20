#ifndef MATCH_INDEX_H
#define MATCH_INDEX_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <mutex>

struct Match {
    std::string file;
    size_t byteOffset;
    std::string keyword;
    std::string context;

    bool operator==(const Match& other) const {
        return file == other.file &&
               byteOffset == other.byteOffset &&
               keyword == other.keyword;
    }
};

// Custom hash function for unordered_set<Match>
namespace std {
    template<>
    struct hash<Match> {
        std::size_t operator()(const Match& m) const {
            return std::hash<std::string>()(m.file) ^
                   std::hash<std::string>()(m.keyword) ^
                   std::hash<size_t>()(m.byteOffset);
        }
    };
}

class MatchIndex {
private:
    std::unordered_map<std::string, std::vector<Match>> keywordToMatches;
    std::unordered_map<std::string, std::vector<Match>> fileToMatches;
    mutable std::mutex mtx;

public:
    void updateMatchesForFile(const std::string& file, const std::vector<Match>& newMatches) {
        std::lock_guard<std::mutex> lock(mtx);

        auto& oldMatches = fileToMatches[file];
        std::unordered_set<Match> newSet(newMatches.begin(), newMatches.end());

        size_t evicted = 0, added = 0;

        // Remove stale matches from keywordToMatches
        for (const auto& old : oldMatches) {
            if (!newSet.count(old)) {
                auto& vec = keywordToMatches[old.keyword];
                vec.erase(std::remove_if(vec.begin(), vec.end(), [&](const Match& m) {
                    return m.file == file && m.byteOffset == old.byteOffset;
                }), vec.end());
                ++evicted;
            }
        }

        // Add new matches to keywordToMatches
        for (const auto& m : newMatches) {
            keywordToMatches[m.keyword].push_back(m);
            ++added;
        }

        fileToMatches[file] = newMatches;

        std::cout << "[cache] File: " << file
                  << " | Matches added: " << added
                  << " | Matches evicted: " << evicted
                  << " | Reused: " << (added == 0 && evicted == 0 ? "YES" : "NO")
                  << std::endl;
    }

    const std::vector<Match>& getMatchesForKeyword(const std::string& keyword) const {
        static const std::vector<Match> empty;
        std::lock_guard<std::mutex> lock(mtx);
        auto it = keywordToMatches.find(keyword);
        return (it != keywordToMatches.end()) ? it->second : empty;
    }

    const std::vector<Match>& getMatchesForFile(const std::string& file) const {
        static const std::vector<Match> empty;
        std::lock_guard<std::mutex> lock(mtx);
        auto it = fileToMatches.find(file);
        return (it != fileToMatches.end()) ? it->second : empty;
    }

    bool hasMatchesForFile(const std::string& file) const {
        std::lock_guard<std::mutex> lock(mtx);
        return fileToMatches.find(file) != fileToMatches.end();
    }

    void clear() {
        std::lock_guard<std::mutex> lock(mtx);
        keywordToMatches.clear();
        fileToMatches.clear();
    }
};

#endif
