#pragma once
#include "unicode.h"
#include "../core/matchIndex.h"
#include <tuple>
inline void annotateRanges(std::vector<Match>& matches, const std::string& text) {
    std::vector<std::tuple<size_t, size_t, bool>> endpoints;
    for (size_t i = 0; i < matches.size(); ++i) {
        endpoints.emplace_back(matches[i].byteOffset, i, false);
        endpoints.emplace_back(matches[i].byteOffset + matches[i].keyword.size(), i, true);
    }
    std::sort(endpoints.begin(), endpoints.end());
    size_t pos = 0, line = 0, character = 0; bool valid = true;
    for (const auto& endpoint : endpoints) {
        const size_t offset = std::get<0>(endpoint);
        while (pos < offset && pos < text.size()) {
            size_t n = utf8Length(text, pos);
            if (!n) { valid = false; n = 1; }
            if (text[pos] == '\r') {
                n = pos + 1 < text.size() && text[pos+1] == '\n' ? 2 : 1;
                ++line; character = 0;
            } else if (text[pos] == '\n') { ++line; character = 0; }
            else character += n == 4 ? 2 : 1;
            pos += n;
        }
        auto& match = matches[std::get<1>(endpoint)];
        match.validUtf8 = match.validUtf8 && valid && pos == offset;
        if (std::get<2>(endpoint)) { match.endLine = line; match.endCharacter = character; }
        else { match.startLine = line; match.startCharacter = character; }
    }
}
