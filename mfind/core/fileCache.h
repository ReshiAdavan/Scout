#ifndef FILE_CACHE_H
#define FILE_CACHE_H
#include <string>
#include <unordered_map>
#include <mutex>
#include <cstdint>
#include "../helpers/ignoreRules.h"
class FileCache {
    mutable std::mutex mutex;
    std::unordered_map<std::string, uint64_t> fingerprints;
public:
    static uint64_t fingerprint(const std::string& file);
    bool isChanged(const std::string& file) const;
    bool hasChangedFiles(const std::string& root, const IgnoreRules& ignore) const;
    void update(const std::string& file);
    void update(const std::string& file, uint64_t fingerprint);
};
#endif
