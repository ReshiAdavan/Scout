#pragma once
#include "../core/matchIndex.h"
#include "cli.h"
#include <iomanip>
#include <sstream>
#include <unistd.h>
#include "unicode.h"
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
inline std::string fileUri(const std::string& path) {
    std::ostringstream out; out << "file://";
    for (unsigned char c : std::filesystem::absolute(path).lexically_normal().generic_string()) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || std::string("/-._~").find(c) != std::string::npos) out << c;
        else out << '%' << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << unsigned(c) << std::dec;
    }
    return out.str();
}
inline std::string locationJson(const Match& m) {
    if (!m.validUtf8) throw std::runtime_error("LSP output requires UTF-8 codepoint boundaries: " + m.file);
    return "{\"uri\":" + jsonString(fileUri(m.file)) + ",\"range\":{\"start\":{\"line\":" + std::to_string(m.startLine) +
        ",\"character\":" + std::to_string(m.startCharacter) + "},\"end\":{\"line\":" + std::to_string(m.endLine) +
        ",\"character\":" + std::to_string(m.endCharacter) + "}}}";
}
inline void printResults(const std::vector<Match>& matches, const Options& options, std::ostream& out = std::cout) {
    const bool color = options.color == "always" || (options.color == "auto" && isatty(STDOUT_FILENO));
    if (options.format == "lsp") {
        std::vector<std::string> locations; for (const auto& m : matches) locations.push_back(locationJson(m));
        out << '['; for (size_t i = 0; i < locations.size(); ++i) { if (i) out << ','; out << locations[i]; } out << "]\n"; out.flush(); return;
    }
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
