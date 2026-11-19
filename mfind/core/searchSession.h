#pragma once
#include "fileCache.h"
#include "matchIndex.h"
#include "../helpers/searchHelper.h"
#include "../helpers/cli.h"
#include "../helpers/ranges.h"
#include <future>
#include <map>
#include <set>
#include <tuple>
#include <unistd.h>
#include <limits>

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
    bool ignoreCase = false, wholeWord = false;
    std::string signature;
    static size_t entryBytes(const std::pair<const std::string, Entry>& entry) {
        size_t bytes = sizeof(entry) + entry.first.capacity();
        for (const auto& m : entry.second.matches) bytes += sizeof(Match) + m.file.capacity() + m.keyword.capacity() + m.context.capacity() + m.text.capacity();
        return bytes;
    }
    void loadSnapshot(const std::string& path, const std::string& expected, size_t budget) {
        if (path.empty()) return;
        std::error_code error; const auto bytes = std::filesystem::file_size(path, error);
        if (error || bytes > std::min<size_t>(budget * 2, 128 * 1024 * 1024)) return;
        std::ifstream input(path, std::ios::binary);
        std::string magic, identity;
        size_t count = 0;
        if (!(input >> magic >> std::quoted(identity) >> count) || magic != "SCOUT3" || identity != expected || count > 1000000) return;
        std::map<std::string, Entry> loaded; size_t total = 0;
        for (size_t i = 0; i < count; ++i) {
            std::string file; Entry entry{}; size_t matches = 0;
            if (!(input >> std::quoted(file) >> entry.hash >> matches) || file.size() > 1048576 || matches > 1000000) return;
            for (size_t j = 0; j < matches; ++j) {
                Match m; m.file = file;
                if (!(input >> m.byteOffset >> m.line >> m.column >> std::quoted(m.keyword) >> std::quoted(m.context) >> std::quoted(m.text) >> m.startLine >> m.startCharacter >> m.endLine >> m.endCharacter >> m.validUtf8)) return;
                if (std::find(keywords.begin(), keywords.end(), m.keyword) == keywords.end()) return;
                if (m.endLine < m.startLine || (m.endLine == m.startLine && m.endCharacter < m.startCharacter)) return;
                if (!m.line || !m.column || m.keyword.empty() || m.keyword.size() > 1048576 || m.context.size() > 1048576 || m.text.size() != m.keyword.size() || m.byteOffset > std::numeric_limits<size_t>::max() - m.keyword.size()) return;
                total += sizeof(Match) + m.file.capacity() + m.keyword.capacity() + m.context.capacity() + m.text.capacity();
                if (total > budget) return;
                entry.matches.push_back(std::move(m));
            }
            auto inserted = loaded.emplace(file, std::move(entry));
            if (!inserted.second) return;
            total += sizeof(*inserted.first) + file.capacity(); if (total > budget) return;
        }
        input >> std::ws; if (!input.eof()) return;
        entries = std::move(loaded);
    }
    void saveSnapshot(const std::string& path) const {
        if (path.empty()) return;
        auto parent = std::filesystem::path(path).parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);
        std::string pattern = path + ".tmp.XXXXXX";
        std::vector<char> name(pattern.begin(), pattern.end()); name.push_back(0);
        int fd = mkstemp(name.data()); if (fd < 0) throw std::runtime_error("Cannot create cache temporary file");
        close(fd);
        try {
            std::ofstream out(name.data(), std::ios::binary);
            out << "SCOUT3\n" << std::quoted(signature) << '\n' << entries.size() << '\n';
            for (const auto& entry : entries) {
                out << std::quoted(entry.first) << ' ' << entry.second.hash << ' ' << entry.second.matches.size() << '\n';
                for (const auto& m : entry.second.matches) out << m.byteOffset << ' ' << m.line << ' ' << m.column << ' ' << std::quoted(m.keyword) << ' ' << std::quoted(m.context) << ' ' << std::quoted(m.text) << ' ' << m.startLine << ' ' << m.startCharacter << ' ' << m.endLine << ' ' << m.endCharacter << ' ' << m.validUtf8 << '\n';
            }
            out.close(); if (!out) throw std::runtime_error("Cannot save cache snapshot");
            std::filesystem::rename(name.data(), path);
        } catch (...) { std::filesystem::remove(name.data()); throw; }
    }
public:
    size_t scannedFiles = 0, cacheHits = 0, scannedBytes = 0, retainedBytes = 0, evictedFiles = 0;
    std::vector<Match> scan(const Options& options, const std::vector<std::string>& words) try {
        scannedFiles = cacheHits = scannedBytes = 0;
        std::ostringstream identity;
        identity << std::quoted(std::filesystem::absolute(options.root).lexically_normal().string()) << ' ' << options.ignoreCase << options.wholeWord;
        for (const auto& word : words) identity << ' ' << std::quoted(word);
        const bool changedQuery = signature != identity.str() || ignoreCase != options.ignoreCase || wholeWord != options.wholeWord;
        if (changedQuery) {
            entries.clear(); keywords = words; ignoreCase = options.ignoreCase; wholeWord = options.wholeWord;
            signature = identity.str(); loadSnapshot(options.cache, signature, options.indexBytes);
        }
        const size_t previousEntries = entries.size();
        evictedFiles = 0;
        AhoCorasick ac(options.ignoreCase);
        for (const auto& word : words) ac.addKeyword(word);
        ac.build();
        IgnoreRules ignore; ignore.loadFromFile((std::filesystem::path(options.root) / ".scoutignore").string());
        auto files = collectFiles(options.root, ignore);
        files.erase(std::remove_if(files.begin(), files.end(), [&](const std::string& path) {
            if (std::filesystem::absolute(path).lexically_normal() == std::filesystem::absolute(options.keywords).lexically_normal()) return true;
            if (!options.cache.empty() && std::filesystem::absolute(path).lexically_normal() == std::filesystem::absolute(options.cache).lexically_normal()) return true;
            if (!options.config.empty() && std::filesystem::absolute(path).lexically_normal() == std::filesystem::absolute(options.config).lexically_normal()) return true;
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
                    if (!input) {
                        std::error_code error;
                        if (!std::filesystem::exists(file, error) && !error) throw RetryScan("File disappeared during scan: " + file);
                        throw std::runtime_error("Cannot read: " + file);
                    }
                    std::string text((std::istreambuf_iterator<char>(input)), {});
                    if (input.bad()) throw std::runtime_error("Read failed: " + file);
                    if (FileCache::fingerprint(file) != hash) throw RetryScan("File changed during scan: " + file);
                    for (const auto& match : matches) {
                        if (match.byteOffset > text.size() || match.keyword.size() > text.size() - match.byteOffset)
                            throw RetryScan("File shrank during scan: " + file);
                    }
                    if (options.wholeWord) {
                        auto wordChar = [](unsigned char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_'; };
                        matches.erase(std::remove_if(matches.begin(), matches.end(), [&](const Match& m) {
                            const size_t end = m.byteOffset + m.keyword.size();
                            return (m.byteOffset && wordChar(text[m.byteOffset - 1])) || (end < text.size() && wordChar(text[end]));
                        }), matches.end());
                    }
                    std::vector<size_t> lines{0};
                    for (size_t pos = 0; pos < text.size(); ++pos) if (text[pos] == '\n') lines.push_back(pos + 1);
                    for (auto& match : matches) {
                        auto line = std::upper_bound(lines.begin(), lines.end(), match.byteOffset) - lines.begin() - 1;
                        match.line = line + 1; match.column = match.byteOffset - lines[line] + 1;
                        match.text = text.substr(match.byteOffset, match.keyword.size());
                    }
                    annotateRanges(matches, text);
                    if (FileCache::fingerprint(file) != hash) throw RetryScan("File changed during scan: " + file);
                    return Result{file, Entry{hash, std::move(matches)}};
                }));
            }
            for (auto& job : jobs) { auto result = job.get(); entries[result.first] = std::move(result.second); }
        }
        for (const auto& entry : entries) results.insert(results.end(), entry.second.matches.begin(), entry.second.matches.end());
        std::sort(results.begin(), results.end(), [](const Match& a, const Match& b) {
            return std::tie(a.file, a.byteOffset, a.keyword) < std::tie(b.file, b.byteOffset, b.keyword);
        });
        retainedBytes = 0;
        for (const auto& entry : entries) retainedBytes += entryBytes(entry);
        while (retainedBytes > options.indexBytes && !entries.empty()) {
            retainedBytes -= entryBytes(*entries.begin()); entries.erase(entries.begin()); ++evictedFiles;
        }
        if (changedQuery || scannedFiles || previousEntries != entries.size()) saveSnapshot(options.cache);
        return results;
    } catch (const std::filesystem::filesystem_error& error) {
        if (error.code() == std::errc::no_such_file_or_directory) throw RetryScan(error.what());
        throw;
    }
};
