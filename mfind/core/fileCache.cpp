#include "fileCache.h"
#include "../helpers/scanError.h"
#include <fstream>
#include <stdexcept>
uint64_t FileCache::fingerprint(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        std::error_code error;
        if (!std::filesystem::exists(path, error) && !error) throw RetryScan("File disappeared during scan: " + path);
        throw std::runtime_error("Cannot read: " + path);
    }
    uint64_t hash = 14695981039346656037ULL;
    char buffer[65536];
    while (in.read(buffer, sizeof buffer) || in.gcount()) {
        for (std::streamsize i = 0; i < in.gcount(); ++i) { hash ^= static_cast<unsigned char>(buffer[i]); hash *= 1099511628211ULL; }
    }
    if (in.bad()) throw std::runtime_error("Read failed: " + path);
    return hash;
}
bool FileCache::isChanged(const std::string& file) const {
    auto hash = fingerprint(file);
    std::lock_guard<std::mutex> lock(mutex);
    auto it = fingerprints.find(file);
    return it == fingerprints.end() || it->second != hash;
}
bool FileCache::hasChangedFiles(const std::string& root, const IgnoreRules& ignore) const {
    for (const auto& file : collectFiles(root, ignore)) if (isChanged(file)) return true;
    return false;
}
void FileCache::update(const std::string& file) { update(file, fingerprint(file)); }
void FileCache::update(const std::string& file, uint64_t hash) {
    std::lock_guard<std::mutex> lock(mutex);
    fingerprints[file] = hash;
}
