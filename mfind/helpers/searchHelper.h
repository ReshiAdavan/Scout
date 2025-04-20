#ifndef SEARCH_HELPER_H
#define SEARCH_HELPER_H

#include <fstream>
#include <string>
#include <functional>
#include "../aho/ahoCorasick.h"
#include "formatHelper.h"
#include "chunkScanner.h"

inline void searchFile(const std::string& filename,
                       const AhoCorasick& ac,
                       std::function<void(const std::string&, size_t, const std::string&, const std::string&)> callback,
                       size_t chunkSize = 8192) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    size_t overlap = ac.getMaxPatternLength() - 1;
    std::vector<char> buffer(chunkSize + overlap);
    size_t offset = 0;

    while (!file.eof()) {
        if (offset != 0) {
            std::copy(buffer.end() - overlap, buffer.end(), buffer.begin());
        }

        file.read(buffer.data() + (offset != 0 ? overlap : 0), chunkSize);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead == 0) break;

        size_t total = (offset == 0 ? bytesRead : bytesRead + overlap);
        std::string chunk(buffer.data(), total);

        ChunkScanner::scan(chunk, ac, [&](size_t pos, const std::string& word, const std::string& context) {
            callback(filename, pos, word, context);
        }, offset);

        offset += bytesRead;
    }

    file.close();
}

#endif
