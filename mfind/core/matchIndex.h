#ifndef MATCH_INDEX_H
#define MATCH_INDEX_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <mutex>

struct Match {
    std::string file;
    size_t byteOffset;
    std::string keyword;
    std::string context;
    size_t line = 1, column = 1;
    std::string text{}, id{}, tag{};

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

        for (auto it = keywordToMatches.begin(); it != keywordToMatches.end();) {
            auto& values = it->second;
            values.erase(std::remove_if(values.begin(), values.end(), [&](const Match& m) {
                return m.file == file;
            }), values.end());
            if (values.empty()) it = keywordToMatches.erase(it); else ++it;
        }
        std::unordered_set<Match> seen;
        std::vector<Match> unique;
        for (const auto& match : newMatches) {
            if (seen.insert(match).second) {
                unique.push_back(match);
                keywordToMatches[match.keyword].push_back(match);
            }
        }
        if (unique.empty()) fileToMatches.erase(file);
        else fileToMatches[file] = std::move(unique);
    }

    void removeFile(const std::string& file) {
        updateMatchesForFile(file, {});

    }

    std::vector<Match> getMatchesForKeyword(const std::string& keyword) const {
        static const std::vector<Match> empty;
        std::lock_guard<std::mutex> lock(mtx);
        auto it = keywordToMatches.find(keyword);
        return (it != keywordToMatches.end()) ? it->second : empty;
    }

    std::vector<Match> getMatchesForFile(const std::string& file) const {
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
