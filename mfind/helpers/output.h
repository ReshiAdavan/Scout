#pragma once
#include "../core/matchIndex.h"
#include "cli.h"
#include <iomanip>
#include <sstream>
#include <unistd.h>
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
inline std::string jsonString(const std::string& value) {
    std::ostringstream out; out << '"';
    for (size_t i = 0; i < value.size();) {
        unsigned char c = value[i]; size_t n = utf8Length(value, i);
        if (c == '"' || c == '\\') { out << '\\' << c; ++i; }
        else if (c < 32 || !n) { out << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << unsigned(c) << std::dec; ++i; }
        else { out << value.substr(i, n); i += n; }
    }
    return out.str() + '"';
}
inline std::string csvString(const std::string& value) {
    std::string out = "\"";
    for (char c : value) { out += c; if (c == '"') out += '"'; }
    return out + '"';
}
inline std::string matchJson(const Match& m) {
    return "{\"id\":" + jsonString(m.id.empty() ? m.keyword : m.id) + ",\"tag\":" + jsonString(m.tag) + ",\"file\":" + jsonString(m.file) + ",\"keyword\":" + jsonString(m.keyword) +
           ",\"byteOffset\":" + std::to_string(m.byteOffset) + ",\"line\":" + std::to_string(m.line) +
           ",\"column\":" + std::to_string(m.column) + ",\"text\":" + jsonString(m.text) +
           ",\"context\":" + jsonString(m.context) + "}";
}
inline void printResults(const std::vector<Match>& matches, const Options& options, std::ostream& out = std::cout) {
    const bool color = options.color == "always" || (options.color == "auto" && isatty(STDOUT_FILENO));
    if (options.format == "json") out << "{\"matches\":[";
    if (options.format == "csv") out << "file,keyword,byteOffset,line,column,text,context,id,tag\n";
    for (size_t i = 0; i < matches.size(); ++i) {
        const auto& m = matches[i];
        if (options.format == "json") { if (i) out << ','; out << matchJson(m); }
        else if (options.format == "jsonl") out << matchJson(m) << '\n';
        else if (options.format == "csv") out << csvString(m.file) << ',' << csvString(m.keyword) << ',' << m.byteOffset << ',' << m.line << ',' << m.column << ',' << csvString(m.text) << ',' << csvString(m.context) << ',' << csvString(m.id.empty() ? m.keyword : m.id) << ',' << csvString(m.tag) << '\n';
        else if (options.format == "editor") out << m.file << ':' << m.line << ':' << m.column << ':' << m.text << '\n';
        else out << m.file << ':' << m.line << ':' << m.column << " | " << (color ? "\033[36m" : "") << m.keyword << (color ? "\033[0m" : "") << " | " << m.context << '\n';
    }
    if (options.format == "json") out << "]}\n";
    out.flush();
}
