#ifndef CHUNK_SCANNER_H
#define CHUNK_SCANNER_H

#include <string>
#include <functional>
#include <vector>
#include <thread>
#include <future>
#include <algorithm>
#include "../aho/ahoCorasick.h"
#include "formatHelper.h"

class ChunkScanner {
public:
    static void scan(const std::string& chunk,
                     const AhoCorasick& ac,
                     std::function<void(size_t, const std::string&, const std::string&)> callback,
                     size_t baseOffset = 0,
                     size_t threadCount = std::thread::hardware_concurrency(),
                     size_t contextSize = 30) {
        
        size_t chunkLength = chunk.size();
        size_t sliceSize = std::max<size_t>(chunkLength / threadCount, 4096);
        std::vector<std::future<void>> futures;

        for (size_t i = 0; i < chunkLength; i += sliceSize) {
            size_t start = i;
            size_t end = std::min(chunkLength, i + sliceSize + ac.getMaxPatternLength());

            futures.push_back(std::async(std::launch::async, [&, start, end] {
                std::string slice = chunk.substr(start, end - start);
                auto matches = ac.search(slice);
                for (const auto& [localPos, word] : matches) {
                    size_t absolute = baseOffset + start + localPos;
                    std::string context = extractContext(chunk, localPos + start, word.length(), contextSize);
                    callback(absolute, word, context);
                }
            }));
        }

        for (auto& f : futures) f.get();
    }
};

#endif
