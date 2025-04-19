#ifndef SEARCH_HELPER_H
#define SEARCH_HELPER_H

#include <fstream>
#include <string>
#include <vector>
#include <iostream>
#include "../aho/ahoCorasick.h"
#include "configHelper.h"

inline void searchFile(const std::string& filename,
                       const AhoCorasick& ac,
                       std::function<void(const std::string&, size_t, const std::string&, const std::string&)> onMatch) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;

    const size_t overlap = 100;
    const size_t chunkSize = 8192;
    std::vector<char> buffer(chunkSize + overlap);

    size_t offset = 0;
    while (!file.eof()) {
        if (offset != 0) {
            std::copy(buffer.end() - overlap, buffer.end(), buffer.begin());
        }

        file.read(buffer.data() + (offset != 0 ? overlap : 0), chunkSize);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead == 0) break;

        size_t totalSize = (offset == 0 ? bytesRead : bytesRead + overlap);
        std::string chunkText(buffer.data(), totalSize);

        auto matches = ac.search(chunkText);
        for (const auto& [pos, word] : matches) {
            size_t absPos = offset + pos;

            // Get context
            size_t ctxSize = 30;
            size_t ctxStart = (pos > ctxSize) ? (pos - ctxSize) : 0;
            size_t ctxEnd = std::min(pos + word.length() + ctxSize, chunkText.length());
            std::string context = chunkText.substr(ctxStart, ctxEnd - ctxStart);

            onMatch(filename, absPos, word, context);
        }

        offset += bytesRead;
    }

    file.close();
}

#endif
