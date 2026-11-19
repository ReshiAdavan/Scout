#pragma once
#include "fileCache.h"
#include "matchIndex.h"
#include "../helpers/searchHelper.h"
#include "../helpers/cli.h"
#include <future>
#include <map>
#include <set>
#include <tuple>

inline std::vector<std::string> loadKeywords(const std::string& path) {
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read keywords: " + path);
    std::vector<std::string> words; std::set<std::string> seen;
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!line.empty() && seen.insert(line).second) words.push_back(line);
    }
    if (file.bad()) throw std::runtime_error("Cannot read keywords: " + path);
    return words;
}
class SearchSession {
    struct Entry { uint64_t hash; std::vector<Match> matches; };
    std::map<std::string, Entry> entries;
    std::vector<std::string> keywords;
public:
    size_t scannedFiles = 0, cacheHits = 0, scannedBytes = 0;
    std::vector<Match> scan(const Options& options, const std::vector<std::string>& words) {
        scannedFiles = cacheHits = scannedBytes = 0;
        if (keywords != words) { entries.clear(); keywords = words; }
        AhoCorasick ac;
        for (const auto& word : words) ac.addKeyword(word);
        ac.build();
        IgnoreRules ignore; ignore.loadFromFile((std::filesystem::path(options.root) / ".scoutignore").string());
        auto files = collectFiles(options.root, ignore);
        files.erase(std::remove_if(files.begin(), files.end(), [&](const std::string& path) {
            if (std::filesystem::absolute(path).lexically_normal() == std::filesystem::absolute(options.keywords).lexically_normal()) return true;
            return !options.extensions.empty() && std::find(options.extensions.begin(), options.extensions.end(), std::filesystem::path(path).extension().string()) == options.extensions.end();
        }), files.end());
        const std::set<std::string> live(files.begin(), files.end());
        for (auto it = entries.begin(); it != entries.end();) { if (!live.count(it->first)) it = entries.erase(it); else ++it; }
        std::vector<Match> results;
        for (size_t start = 0; start < files.size(); start += options.threads) {
            using Result = std::pair<std::string, Entry>;
            std::vector<std::future<Result>> jobs;
            for (size_t i = start; i < std::min(files.size(), start + options.threads); ++i) {
                const auto& file = files[i]; const auto hash = FileCache::fingerprint(file);
                auto cached = entries.find(file);
                if (cached != entries.end() && cached->second.hash == hash) { ++cacheHits; continue; }
                ++scannedFiles; scannedBytes += std::filesystem::file_size(file);
                jobs.push_back(std::async(std::launch::async, [&, file, hash] {
                    auto matches = searchFile(file, ac);
                    std::ifstream input(file, std::ios::binary);
                    std::string text((std::istreambuf_iterator<char>(input)), {});
                    if (input.bad()) throw std::runtime_error("Read failed: " + file);
                    std::vector<size_t> lines{0};
                    for (size_t pos = 0; pos < text.size(); ++pos) if (text[pos] == '\n') lines.push_back(pos + 1);
                    for (auto& match : matches) {
                        auto line = std::upper_bound(lines.begin(), lines.end(), match.byteOffset) - lines.begin() - 1;
                        match.line = line + 1; match.column = match.byteOffset - lines[line] + 1;
                        match.text = text.substr(match.byteOffset, match.keyword.size());
                    }
                    if (FileCache::fingerprint(file) != hash) throw std::runtime_error("File changed during scan; retry: " + file);
                    return Result{file, Entry{hash, std::move(matches)}};
                }));
            }
            for (auto& job : jobs) { auto result = job.get(); entries[result.first] = std::move(result.second); }
        }
        for (const auto& entry : entries) results.insert(results.end(), entry.second.matches.begin(), entry.second.matches.end());
        std::sort(results.begin(), results.end(), [](const Match& a, const Match& b) {
            return std::tie(a.file, a.byteOffset, a.keyword) < std::tie(b.file, b.byteOffset, b.keyword);
        });
        return results;
    }
};
