#ifndef BOYERMOORE_H
#define BOYERMOORE_H

#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <iostream>

class BoyerMoore {
private:
    std::string pattern;
    std::unordered_map<char, int> badCharTable;
    std::vector<int> goodSuffixTable;

    void preprocessBadCharTable() {
        for (int i = 0; i < 256; i++) {
            badCharTable[static_cast<char>(i)] = -1;
        }

        for (size_t i = 0; i < pattern.size(); i++) {
            badCharTable[pattern[i]] = i;
        }
    }

    void computeSuffixArray(std::vector<int>& suffix) {
        int m = pattern.length();
        suffix[m - 1] = m;
        int g = m - 1, f = 0;

        for (int i = m - 2; i >= 0; i--) {
            if (i > g && suffix[i + m - 1 - f] < i - g) {
                suffix[i] = suffix[i + m - 1 - f];
            } else {
                if (i < g) g = i;
                f = i;
                while (g >= 0 && pattern[g] == pattern[g + m - 1 - f]) {
                    g--;
                }
                suffix[i] = f - g;
            }
        }
    }

    void preprocessGoodSuffixTable() {
        int m = pattern.length();
        goodSuffixTable.resize(m, m);

        std::vector<int> suffix(m, 0);
        computeSuffixArray(suffix);

        // Suffix is also a prefix
        for (int i = m - 1; i >= 0; --i) {
            if (suffix[i] == i + 1) {
                for (int j = 0; j < m - 1 - i; ++j) {
                    if (goodSuffixTable[j] == m) {
                        goodSuffixTable[j] = m - 1 - i;
                    }
                }
            }
        }

        // Substring pattern[i+1:] matches a suffix
        for (int i = 0; i < m - 1; ++i) {
            goodSuffixTable[m - 1 - suffix[i]] = m - 1 - i;
        }
    }

public:
    BoyerMoore(const std::string& pat) : pattern(pat) {
        preprocessBadCharTable();
        preprocessGoodSuffixTable();
    }

    int find(const std::string& text) {
        int m = pattern.length();
        int n = text.length();
        if (m == 0 || m > n) return -1;

        int s = 0;
        while (s <= n - m) {
            int j = m - 1;
            while (j >= 0 && pattern[j] == text[s + j]) {
                j--;
            }

            if (j < 0) {
                return s; // Match found
            } else {
                int badCharShift = std::max(1, j - badCharTable[text[s + j]]);
                int goodSuffixShift = goodSuffixTable[j];
                s += std::max(badCharShift, goodSuffixShift);
            }
        }

        return -1;
    }

    std::vector<size_t> findAll(const std::string& text) {
        std::vector<size_t> positions;
        int m = pattern.length();
        int n = text.length();
        if (m == 0 || m > n) return positions;

        int s = 0;
        while (s <= n - m) {
            int j = m - 1;
            while (j >= 0 && pattern[j] == text[s + j]) {
                j--;
            }

            if (j < 0) {
                positions.push_back(s);
                s += goodSuffixTable[0];  // Shift after match
            } else {
                int badCharShift = std::max(1, j - badCharTable[text[s + j]]);
                int goodSuffixShift = goodSuffixTable[j];
                s += std::max(badCharShift, goodSuffixShift);
            }
        }

        return positions;
    }
};

#endif // BOYERMOORE_H
