#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <tuple>
#include "../helpers/fileHelper.h"
#include "fuzzy.h"

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <fuzzy_query>\n";
        return 1;
    }

    std::string query = argv[1];
    auto files = getAllFiles(".");

    std::vector<std::pair<std::string, int>> matches;

    for (const auto& file : files) {
        int score = fuzzyScore(query, file);
        if (score >= 0) {
            matches.emplace_back(file, score);
        }
    }

    std::sort(matches.begin(), matches.end(), [](auto& a, auto& b) {
        return a.second > b.second;
    });

    for (const auto& [file, score] : matches) {
        std::cout << file << "  (score: " << score << ")\n";
    }

    return 0;
}
