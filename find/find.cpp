#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include "boyerMoore.h"

void searchFile(const std::string& filename, const std::string& searchPattern) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << std::endl;
        return;
    }
    
    std::string content;
    std::string line;
    int lineNumber = 0;
    
    BoyerMoore bm(searchPattern);
    
    // line-by-line search
    while (std::getline(file, line)) {
        lineNumber++;
        std::vector<size_t> positions = bm.findAll(line);
        for (size_t pos : positions) {
            // Determine how much context to show before and after the match
            size_t contextBefore = std::min(pos, size_t(20));
            size_t contextAfter = std::min(line.length() - pos - searchPattern.length(), size_t(20));
            
            // Extract the context
            std::string before = line.substr(pos - contextBefore, contextBefore);
            std::string after = line.substr(pos + searchPattern.length(), contextAfter);
            
            std::cout << "Match found at line " << lineNumber
                      << ", position " << pos + 1
                      << ": ..." << before << "[" << line.substr(pos, searchPattern.length()) << "]" 
                      << after << "..." << std::endl;
        }
    }
    
    file.close();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <filename> <search_pattern>" << std::endl;
        return 1;
    }
    
    std::string filename = argv[1];
    std::string pattern = argv[2];
    
    std::cout << "Searching for '" << pattern << "' in " << filename << std::endl;
    searchFile(filename, pattern);
    
    return 0;
}