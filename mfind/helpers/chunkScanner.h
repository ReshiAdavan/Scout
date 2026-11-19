#ifndef CHUNK_SCANNER_H
#define CHUNK_SCANNER_H

#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <future>
#include <algorithm>
#include "../core/ahoCorasick.h"
#include "formatHelper.h"

class ChunkScanner {
public:
    static void scan(const std::string& chunk,
                     const AhoCorasick& ac,
                     std::function<void(size_t, const std::string&, const std::string&)> callback,
                     size_t baseOffset = 0,
                     size_t threadCount = std::thread::hardware_concurrency(),
                     size_t contextSize = 30) {
        
        if (chunk.empty() || ac.getMaxPatternLength() == 0) return;
        const size_t chunkLength = chunk.size();
        threadCount = std::max<size_t>(1, threadCount);
        const size_t sliceSize = std::max<size_t>(chunkLength / threadCount, 4096);
        using Matches = std::vector<std::pair<size_t, std::string>>;
        std::vector<std::future<Matches>> futures;
        for (size_t start = 0; start < chunkLength; start += sliceSize) {
            const size_t owned = std::min(sliceSize, chunkLength - start);
            const size_t extra = std::min(ac.getMaxPatternLength() - 1, chunkLength - start - owned);
            futures.push_back(std::async(std::launch::async, [&, start, owned, extra] {
                Matches result;
                for (const auto& match : ac.search(chunk.substr(start, owned + extra))) {
                    if (match.first < owned) result.emplace_back(start + match.first, match.second);
                }
                return result;
            }));
        }
        // Callbacks run serially on the caller, never on scanning workers.
        for (auto& future : futures) {
            for (const auto& match : future.get()) {
                callback(baseOffset + match.first, match.second,
                         extractContext(chunk, match.first, match.second.size(), contextSize));
            }
        }
    }
};

#endif
