#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include "ahoCorasick.h"

// ANSI colors
std::vector<std::string> colors = {
    "\033[31m", // Red
    "\033[32m", // Green
    "\033[33m", // Yellow
    "\033[34m", // Blue
    "\033[35m", // Magenta
    "\033[36m", // Cyan
};

std::string resetColor = "\033[0m";

void searchFile(const std::string& filename, const AhoCorasick& ac, 
                const std::unordered_map<std::string, std::string>& colorMap,
                size_t chunkSize = 8192) {
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    const size_t overlap = 100;
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
            size_t absolutePos = offset + pos;

            // Context window
            size_t contextSize = 30;
            size_t start = (pos > contextSize) ? (pos - contextSize) : 0;
            size_t end = std::min(pos + word.length() + contextSize, chunkText.length());
            std::string context = chunkText.substr(start, end - start);

            // Color match
            auto it = colorMap.find(word);
            std::string color = (it != colorMap.end()) ? it->second : "\033[37m";

            // Output format: filename | byte offset | colored word | context
            std::cout << filename
                      << " | byte " << absolutePos
                      << " | " << color << word << resetColor
                      << " | ..." << context << "..." << std::endl;
        }

        offset += bytesRead;
    }

    file.close();
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <path> <keyword1> [keyword2] ...\n";
        return 1;
    }

    std::string path = argv[1];
    AhoCorasick ac;

    // Add keywords to Aho-Corasick
    for (int i = 2; i < argc; ++i) {
        ac.addKeyword(argv[i]);
    }
    ac.build();

    // Assign colors to each keyword
    std::unordered_map<std::string, std::string> colorMap;
    int colorIndex = 0;
    for (const auto& word : ac.getKeywords()) {
        colorMap[word] = colors[colorIndex++ % colors.size()];
    }

    if (std::filesystem::is_regular_file(path)) {
        searchFile(path, ac, colorMap);
    } else if (std::filesystem::is_directory(path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                searchFile(entry.path().string(), ac, colorMap);
            }
        }
    } else {
        std::cerr << "Invalid path.\n";
        return 1;
    }

    return 0;
}
