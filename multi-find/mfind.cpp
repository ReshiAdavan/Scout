#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <unordered_map>
#include "../helpers/fileHelper.h"
#include "../helpers/formatHelper.h"
#include "../helpers/configHelper.h"
#include "ahoCorasick.h"

void searchFile(const std::string& filename, const AhoCorasick& ac,
                const std::unordered_map<std::string, std::string>& colorMap) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }

    std::vector<char> buffer(DEFAULT_CHUNK_SIZE + DEFAULT_OVERLAP_SIZE);
    size_t offset = 0;

    while (!file.eof()) {
        if (offset != 0) {
            std::copy(buffer.end() - DEFAULT_OVERLAP_SIZE, buffer.end(), buffer.begin());
        }

        file.read(buffer.data() + (offset != 0 ? DEFAULT_OVERLAP_SIZE : 0), DEFAULT_CHUNK_SIZE);
        std::streamsize bytesRead = file.gcount();
        if (bytesRead == 0) break;

        size_t totalSize = (offset == 0 ? bytesRead : bytesRead + DEFAULT_OVERLAP_SIZE);
        std::string chunkText(buffer.data(), totalSize);

        auto matches = ac.search(chunkText);
        for (const auto& [pos, word] : matches) {
            size_t absolutePos = offset + pos;
            std::string context = extractContext(chunkText, pos, word.size(), CONTEXT_CHARS);
            printMatch(filename, absolutePos, word, context, colorMap);
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

    // Load
    for (int i = 2; i < argc; ++i) {
        ac.addKeyword(argv[i]);
    }
    ac.build();

    // Color => Keyword
    auto colorMap = buildColorMap(ac.getKeywords());

    auto files = getAllFiles(path);
    for (const auto& file : files) {
        searchFile(file, ac, colorMap);
    }

    return 0;
}
