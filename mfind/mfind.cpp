#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_set>
#include <vector>
#include <string>
#include <unordered_map>
#include <csignal>
#include <chrono>
#include <thread>
#include <memory>
#include <set>
#include "../core/ahoCorasick.h"
#include "../core/fileCache.h"
#include "../helpers/searchHelper.h"
#include "../helpers/formatHelper.h"
#include "../helpers/configHelper.h"
#include "../helpers/ignoreRules.h"
#include "walkers/walker.h"
#include "queues/fileQueue.h"
#include "workers/workerPool.h"

volatile std::sig_atomic_t running = 1;

void signalHandler(int) {
    running = false;
}

std::vector<std::string> loadKeywords(const std::string& path) {
    std::vector<std::string> result;
    std::ifstream file(path);
    if (!file) throw std::runtime_error("Cannot read keywords: " + path);
    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty()) result.push_back(line);
    }
    return result;
}

std::vector<std::string> diffKeywords(const std::vector<std::string>& current, const std::unordered_set<std::string>& known) {
    std::vector<std::string> diff;
    for (const auto& kw : current) {
        if (!known.count(kw)) diff.push_back(kw);
    }
    return diff;
}

void runSearchRound(const std::string& root, AhoCorasick& ac, FileCache& cache, std::unordered_map<std::string, std::string>& colorMap,
        const IgnoreRules& ignore, MatchIndex& matchIndex, std::mutex& matchIndexMutex, bool force = false) {
    FileQueue fileQueue;
    Walker walker(2);
    walker.setFileCallback([&](const std::string& file) {
        std::filesystem::path p = file;
        if (ignore.shouldIgnore(p)) {
            return;
        }
        fileQueue.push(file);
    });

    WorkerPool pool(4, fileQueue, [&](const std::string& file) {
        if (force || cache.isChanged(file)) {
            std::cout << "[worker] Scanning (updated): " << file << "\n";
            auto matches = searchFile(file, ac);
        
            for (const auto& match : matches) {
                printMatch(match.file, match.byteOffset, match.keyword, match.context, colorMap);
            }
        
            {
                std::lock_guard<std::mutex> lock(matchIndexMutex);
                matchIndex.updateMatchesForFile(file, matches);
            }
        
            cache.update(file);
        } else {
            std::cout << "[worker] Cached (unchanged): " << file << "\n";
            std::lock_guard<std::mutex> lock(matchIndexMutex);
            const auto& cached = matchIndex.getMatchesForFile(file);
            for (const auto& match : cached) {
                printMatch(match.file, match.byteOffset, match.keyword, match.context, colorMap);
            }
        }
    });

    walker.start(root);
    pool.start();

    walker.join();
    fileQueue.setFinished();
    pool.join();
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <root_dir>\n";
        return 1;
    }

    std::signal(SIGINT, signalHandler);

    std::string root = argv[1];
    std::string keywordFile = "keywords.txt";

    FileCache cache;
    IgnoreRules ignore;
    ignore.loadFromFile();
    MatchIndex matchIndex;
    std::mutex matchIndexMutex;
    std::unordered_map<std::string, std::string> colorMap;
    std::vector<std::string> previousKeywords;
    std::set<std::string> previousFiles;
    bool first = true;
    std::unique_ptr<AhoCorasick> ac;
    // Rebuild the result index at startup. Legacy on-disk match data has no
    // query/root identity and cannot safely be trusted across sessions.
    try {
        while (running) {
            auto loaded = loadKeywords(keywordFile);
            const bool changed = first || loaded != previousKeywords;
            if (changed) {
                ac = std::make_unique<AhoCorasick>();
                std::unordered_set<std::string> unique;
                for (const auto& word : loaded) if (unique.insert(word).second) ac->addKeyword(word);
                ac->build();
                colorMap = buildColorMap(loaded);
                matchIndex.clear();
                previousKeywords = loaded;
            }
            std::set<std::string> files;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
                if (entry.is_regular_file() && !ignore.shouldIgnore(entry.path())) files.insert(entry.path().string());
            }
            for (const auto& file : previousFiles) if (!files.count(file)) matchIndex.removeFile(file);
            if (changed || files != previousFiles || cache.hasChangedFiles(root, ignore)) {
                runSearchRound(root, *ac, cache, colorMap, ignore, matchIndex, matchIndexMutex, changed);
            }
            previousFiles = std::move(files);
            first = false;
            for (int i = 0; i < 50 && running; ++i) std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n";
        return 2;
    }
    return 0;
}
