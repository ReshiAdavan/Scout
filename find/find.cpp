#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include "boyerMoore.h"

void searchFile(const std::string& filename, const BoyerMoore& bm, const std::string& searchPattern, size_t chunkSize = 8192) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    const size_t overlap = searchPattern.size() - 1;
    std::vector<char> buffer(chunkSize + overlap);

    size_t offset = 0;
    size_t chunkCount = 0;
    while (!file.eof()) {
        // Adjust offset for overlap
        if (offset != 0) {
            // Move previous overlap to beginning of buffer
            std::copy(buffer.end() - overlap, buffer.end(), buffer.begin());
        }

        // Read next chunk (after overlap)
        file.read(buffer.data() + (offset != 0 ? overlap : 0), chunkSize);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead == 0) break;

        // Total valid size to search in this buffer
        size_t totalSize = (offset == 0 ? bytesRead : bytesRead + overlap);
        std::string chunkText(buffer.data(), totalSize);

        // Search within chunk
        std::vector<size_t> positions = bm.findAll(chunkText);
        for (size_t pos : positions) {
            size_t absolutePos = offset + pos;
        
            // Context window size
            size_t context = 30;
            size_t contextStart = (pos > context) ? (pos - context) : 0;
        
            size_t available = chunkText.size() - contextStart;
            size_t contextLen = std::min(available, context * 2 + searchPattern.length());
        
            std::string contextStr = chunkText.substr(contextStart, contextLen);
        
            // Position of the match within the context string
            size_t matchPosInContext = pos - contextStart;
        
            // Split before-match, match, and after-match
            std::string before = contextStr.substr(0, matchPosInContext);
            std::string match = contextStr.substr(matchPosInContext, searchPattern.length());
            std::string after = contextStr.substr(matchPosInContext + searchPattern.length());
        
            std::cout << "Match at byte " << absolutePos << ":\n";
            std::cout << "..." << before
                      << "\033[34m" << match << "\033[0m"
                      << after << "...\n\n";
        }

        offset += bytesRead;
        chunkCount++;
    }

    file.close();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <path> <search_pattern>" << std::endl;
        return 1;
    }

    std::string path = argv[1];
    std::string pattern = argv[2];

    const BoyerMoore bm(pattern);

    if (std::filesystem::is_regular_file(path)) {
        std::cout << "**Searching for '" << pattern << "' in file: " << path << "**\n\n";
        searchFile(path, bm, pattern);
    } else if (std::filesystem::is_directory(path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                std::cout << "**Searching for '" << pattern << "' in file: " << entry.path() << "**\n\n";
                searchFile(entry.path().string(), bm, pattern);
            }
        }
    } else {
        std::cerr << "Error: Path is not a file or directory.\n";
        return 1;
    }

    return 0;
}
