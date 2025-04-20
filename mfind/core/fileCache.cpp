#include "fileCache.h"

FileCache::FileCache() {
    load();
}

bool FileCache::isChanged(const std::string& filePath) const {
    try {
        auto current = std::filesystem::last_write_time(filePath);
        auto it = timestamps.find(filePath);
        return (it == timestamps.end() || it->second != current);
    } catch (...) {
        return true;
    }
}

bool FileCache::hasChangedFiles(const std::string& root, const IgnoreRules& ignore) const {
    for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) {
        if (!entry.is_regular_file()) continue;
        std::string path = entry.path().string();
        if (ignore.shouldIgnore(entry.path())) continue;

        try {
            auto current = std::filesystem::last_write_time(entry);
            auto it = timestamps.find(path);
            if (it == timestamps.end() || it->second != current) {
                return true; // found >1 changed file
            }
        } catch (...) {
            continue;
        }
    }
    return false;
}


void FileCache::update(const std::string& filePath) {
    try {
        auto current = std::filesystem::last_write_time(filePath);
        timestamps[filePath] = current;
    } catch (...) {
        std::cerr << "Error updating cache for file: " << filePath << std::endl;
    }
}

void FileCache::load() {
    std::ifstream file(cacheFilePath, std::ios::binary);
    if (!file.is_open()) return;

    size_t count;
    file.read(reinterpret_cast<char*>(&count), sizeof(size_t));
    for (size_t i = 0; i < count; ++i) {
        size_t pathLen;
        file.read(reinterpret_cast<char*>(&pathLen), sizeof(size_t));

        std::string path(pathLen, '\0');
        file.read(&path[0], pathLen);

        std::filesystem::file_time_type time;
        file.read(reinterpret_cast<char*>(&time), sizeof(time));

        timestamps[path] = time;
    }
    file.close();
}

void FileCache::save() const {
    std::filesystem::create_directory(".cache");
    std::ofstream file(cacheFilePath, std::ios::binary);
    if (!file.is_open()) return;

    size_t count = timestamps.size();
    file.write(reinterpret_cast<const char*>(&count), sizeof(size_t));
    for (const auto& [path, time] : timestamps) {
        size_t pathLen = path.size();
        file.write(reinterpret_cast<const char*>(&pathLen), sizeof(size_t));
        file.write(path.c_str(), pathLen);
        file.write(reinterpret_cast<const char*>(&time), sizeof(time));
    }
    file.close();
}
