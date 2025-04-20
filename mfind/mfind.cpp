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
#include "../core/ahoCorasick.h"
#include "../core/fileCache.h"
#include "../helpers/searchHelper.h"
#include "../helpers/formatHelper.h"
#include "../helpers/configHelper.h"
#include "../helpers/ignoreRules.h"
#include "walkers/walker.h"
#include "queues/fileQueue.h"
#include "workers/workerPool.h"

std::atomic<bool> running = true;

void signalHandler(int) {
    std::cout << "\n[!] Caught signal. Shutting down...\n";
    running = false;
}

std::vector<std::string> loadKeywords(const std::string& path) {
    std::vector<std::string> result;
    std::ifstream file(path);
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

void runSearchRound(const std::string& root, AhoCorasick& ac, FileCache& cache,
                    std::unordered_map<std::string, std::string>& colorMap,
                    const IgnoreRules& ignore,
                    bool force = false) {

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
            searchFile(file, ac, [&](const std::string& file, size_t pos, const std::string& word, const std::string& context) {
                printMatch(file, pos, word, context, colorMap);
            });
            cache.update(file);
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

    AhoCorasick ac;
    FileCache cache;
    IgnoreRules ignore;
    ignore.loadFromFile(); // .scoutignore

    std::unordered_set<std::string> knownKeywords;
    std::unordered_map<std::string, std::string> colorMap;

    std::cout << "[+] Starting daemon mode. Watching '" << keywordFile << "'...\n";

    while (running) {
        auto loaded = loadKeywords(keywordFile);
        auto newWords = diffKeywords(loaded, knownKeywords);
        bool keywordSetChanged = !newWords.empty();
        bool filesChanged = cache.hasChangedFiles(root, ignore);

        if (keywordSetChanged || filesChanged) {
            bool force = keywordSetChanged;
            std::cout << "[*] Detected " << newWords.size() << " new keyword(s)...\n";
            for (const auto& w : newWords) {
                ac.addKeyword(w);
                knownKeywords.insert(w);
            }
            ac.build(); // rebuild fail links

            for (const auto& word : newWords) {
                colorMap[word] = defaultColors[colorMap.size() % defaultColors.size()];
            }

            runSearchRound(root, ac, cache, colorMap, ignore, force);
            cache.save();
        }

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }

    std::cout << "[+] Done.\n";
    cache.save();
    return 0;
}
