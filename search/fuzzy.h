#ifndef FUZZY_H
#define FUZZY_H

#include <string>
#include <algorithm>
#include <cctype>

// Check if pattern is a fuzzy subsequence of str
inline bool isFuzzySubsequence(const std::string& pattern, const std::string& str) {
    size_t j = 0;
    for (size_t i = 0; i < str.size() && j < pattern.size(); ++i) {
        if (tolower(str[i]) == tolower(pattern[j])) {
            ++j;
        }
    }
    return j == pattern.size();
}

// Scoring: 100 down
inline int fuzzyScore(const std::string& pattern, const std::string& str) {
    if (!isFuzzySubsequence(pattern, str)) return -1;

    // Boost if match is closer to start or fewer gaps
    int lenDiff = str.length() - pattern.length();
    int score = 100 - lenDiff;
    if (!str.empty() && tolower(str[0]) == tolower(pattern[0])) {
        score += 10;
    }
    return score;
}

#endif
