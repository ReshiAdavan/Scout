#ifndef FILE_HELPER_H
#define FILE_HELPER_H

#include <vector>
#include <string>
#include <filesystem>
#include "configHelper.h"

std::vector<char> buffer(DEFAULT_CHUNK_SIZE + DEFAULT_OVERLAP_SIZE);

std::vector<std::string> getAllFiles(const std::string& path) {
    std::vector<std::string> files;
    if (std::filesystem::is_regular_file(path)) {
        files.push_back(path);
    } else if (std::filesystem::is_directory(path)) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                files.push_back(entry.path().string());
            }
        }
    }
    return files;
}

#endif
