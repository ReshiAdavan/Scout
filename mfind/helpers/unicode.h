#pragma once
#include <string>
// Return valid UTF-8 sequence length; invalid bytes are represented individually.
inline size_t utf8Length(const std::string& text, size_t i) {
    unsigned char c = text[i];
    if (c < 0x80) return 1;
    size_t n = c >= 0xc2 && c <= 0xdf ? 2 : c >= 0xe0 && c <= 0xef ? 3 : c >= 0xf0 && c <= 0xf4 ? 4 : 0;
    if (!n || i + n > text.size()) return 0;
    for (size_t j = 1; j < n; ++j) if ((static_cast<unsigned char>(text[i+j]) & 0xc0) != 0x80) return 0;
    const unsigned char second = text[i+1];
    if ((c == 0xe0 && second < 0xa0) || (c == 0xed && second >= 0xa0) || (c == 0xf0 && second < 0x90) || (c == 0xf4 && second >= 0x90)) return 0;
    return n;
}
