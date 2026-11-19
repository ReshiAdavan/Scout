#ifndef SEARCH_HELPER_H
#define SEARCH_HELPER_H

#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <stdexcept>
#include "../core/ahoCorasick.h"
#include "formatHelper.h"
#include "chunkScanner.h"
#include "../core/matchIndex.h"

// stream-match to callback
inline void searchFile(const std::string& filename,
                       const AhoCorasick& ac,
                       std::function<void(const std::string&, size_t, const std::string&, const std::string&)> callback,
                       size_t chunkSize = 8192) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    if (chunkSize == 0) throw std::invalid_argument("chunk size must be positive");
    if (ac.getMaxPatternLength() == 0) return;
    const size_t overlap = ac.getMaxPatternLength() - 1;
    std::vector<char> buffer(chunkSize);
    std::string tail;
    size_t consumed = 0;
    while (file.read(buffer.data(), chunkSize) || file.gcount() > 0) {
        const size_t count = static_cast<size_t>(file.gcount());
        std::string chunk = tail + std::string(buffer.data(), count);
        const size_t base = consumed - tail.size();
        ChunkScanner::scan(chunk, ac, [&](size_t pos, const std::string& word, const std::string& context) {
            // A match belongs to the read containing its final byte.
            if (pos + word.size() > consumed) callback(filename, pos, word, context);
        }, base);
        consumed += count;
        tail = chunk.substr(chunk.size() - std::min(overlap, chunk.size()));
    }

    file.close();
}

// collect matches for indexing
inline std::vector<Match> searchFile(const std::string& filename,
                                     const AhoCorasick& ac,
                                     size_t chunkSize = 8192) {
    std::vector<Match> matches;
    searchFile(filename, ac, [&](const std::string& file, size_t pos, const std::string& word, const std::string& context) {
        matches.push_back(Match{file, pos, word, context});
    }, chunkSize);
    return matches;
}

#endif
