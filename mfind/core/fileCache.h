#ifndef FILE_CACHE_H
#define FILE_CACHE_H

#include <string>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>
#include "../helpers/ignoreRules.h"

class FileCache {
private:
    std::unordered_map<std::string, std::filesystem::file_time_type> timestamps;
    std::string cacheFilePath = ".cache/mfind.cache";

public:
    FileCache();
    bool isChanged(const std::string& filePath) const;
    bool hasChangedFiles(const std::string& root, const IgnoreRules& ignore) const;
    void update(const std::string& filePath);
    void load();
    void save() const;
};

#endif
