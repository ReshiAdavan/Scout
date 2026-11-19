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
    std::string text;

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
    void save(const std::string& filename) const {
        std::lock_guard<std::mutex> lock(mtx);
        std::ofstream out(filename, std::ios::binary);
        if (!out) {
            std::cerr << "[!] Failed to open " << filename << " for writing.\n";
            return;
        }

        std::cout << "[+] Saving match index to " << filename << "...\n";

        size_t fileCount = fileToMatches.size();
        out.write(reinterpret_cast<const char*>(&fileCount), sizeof(fileCount));

        for (const auto& [file, matches] : fileToMatches) {
            uint32_t fileLen = file.size();
            out.write(reinterpret_cast<const char*>(&fileLen), sizeof(fileLen));
            out.write(file.data(), fileLen);

            size_t matchCount = matches.size();
            out.write(reinterpret_cast<const char*>(&matchCount), sizeof(matchCount));

            for (const auto& m : matches) {
                uint32_t keyLen = m.keyword.size();
                uint32_t ctxLen = m.context.size();

                out.write(reinterpret_cast<const char*>(&m.byteOffset), sizeof(m.byteOffset));
                out.write(reinterpret_cast<const char*>(&keyLen), sizeof(keyLen));
                out.write(m.keyword.data(), keyLen);
                out.write(reinterpret_cast<const char*>(&ctxLen), sizeof(ctxLen));
                out.write(m.context.data(), ctxLen);
            }
        }

        std::cout << "[+] Match index saved successfully.\n";
    }

    void load(const std::string& filename) {
        std::ifstream in(filename, std::ios::binary);
        if (!in) return;

        size_t fileCount;
        in.read(reinterpret_cast<char*>(&fileCount), sizeof(fileCount));

        for (size_t i = 0; i < fileCount; ++i) {
            uint32_t fileLen;
            in.read(reinterpret_cast<char*>(&fileLen), sizeof(fileLen));
            std::string file(fileLen, '\0');
            in.read(&file[0], fileLen);

            size_t matchCount;
            in.read(reinterpret_cast<char*>(&matchCount), sizeof(matchCount));
            std::vector<Match> matches;

            for (size_t j = 0; j < matchCount; ++j) {
                size_t offset;
                in.read(reinterpret_cast<char*>(&offset), sizeof(offset));

                uint32_t keyLen, ctxLen;
                in.read(reinterpret_cast<char*>(&keyLen), sizeof(keyLen));
                std::string keyword(keyLen, '\0');
                in.read(&keyword[0], keyLen);

                in.read(reinterpret_cast<char*>(&ctxLen), sizeof(ctxLen));
                std::string context(ctxLen, '\0');
                in.read(&context[0], ctxLen);

                matches.push_back(Match{file, offset, keyword, context});
            }

            updateMatchesForFile(file, matches);
        }
    }

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
