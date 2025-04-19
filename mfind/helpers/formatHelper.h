#ifndef FORMAT_HELPER_H
#define FORMAT_HELPER_H

#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>
#include "configHelper.h"

inline std::vector<std::string> defaultColors = {
    "\033[31m", "\033[32m", "\033[33m", "\033[34m", "\033[35m", "\033[36m"
};
inline std::string resetColor = "\033[0m";

inline std::unordered_map<std::string, std::string> buildColorMap(const std::vector<std::string>& keywords) {
    std::unordered_map<std::string, std::string> map;
    int i = 0;
    for (const auto& k : keywords) {
        map[k] = defaultColors[i++ % defaultColors.size()];
    }
    return map;
}

inline std::string extractContext(const std::string& text, size_t matchPos, size_t matchLength, size_t contextSize) {
    size_t start = (matchPos > contextSize) ? matchPos - contextSize : 0;
    size_t end = std::min(matchPos + matchLength + contextSize, text.size());
    return text.substr(start, end - start);
}

inline void printMatch(const std::string& filename, size_t byteOffset,
                       const std::string& keyword, const std::string& context,
                       const std::unordered_map<std::string, std::string>& colorMap) {
    std::string color = colorMap.count(keyword) ? colorMap.at(keyword) : "\033[37m";
    std::cout << filename
              << " | byte " << byteOffset
              << " | " << color << keyword << resetColor
              << " | ..." << context << "..." << std::endl;
}

#endif
