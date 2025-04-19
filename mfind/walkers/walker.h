#ifndef WALKER_H
#define WALKER_H

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <functional>
#include <filesystem>
#include <atomic>

namespace fs = std::filesystem;

class DirQueue {
private:
    std::queue<std::string> queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> finished = false;

public:
    void push(const std::string& path) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(path);
        }
        cv.notify_one();
    }

    bool tryPop(std::string& result) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&]() { return !queue.empty() || finished; });
        if (queue.empty()) return false;
        result = queue.front();
        queue.pop();
        return true;
    }

    void setFinished() {
        finished = true;
        cv.notify_all();
    }

    bool isDone() {
        std::lock_guard<std::mutex> lock(mtx);
        return queue.empty() && finished;
    }
};

class Walker {
private:
    int numThreads;
    DirQueue dirQueue;
    std::vector<std::thread> threads;
    std::function<void(const std::string&)> fileCallback;

public:
    Walker(int numThreads = 2) : numThreads(numThreads) {}

    void setFileCallback(std::function<void(const std::string&)> callback) {
        fileCallback = std::move(callback);
    }

    void start(const std::string& rootDir) {
        dirQueue.push(rootDir);
        for (int i = 0; i < numThreads; ++i) {
            threads.emplace_back([this]() {
                std::string dir;
                while (dirQueue.tryPop(dir)) {
                    try {
                        for (const auto& entry : fs::directory_iterator(dir)) {
                            if (entry.is_directory()) {
                                dirQueue.push(entry.path().string());
                            } else if (entry.is_regular_file()) {
                                fileCallback(entry.path().string());
                            }
                        }
                    } catch (const std::exception& e) {
                        std::cerr << "[Walker] Error in " << dir << ": " << e.what() << std::endl;
                    }
                }
            });
        }
    }

    void join() {
        dirQueue.setFinished();
        for (auto& t : threads) {
            if (t.joinable()) t.join();
        }
    }
};

#endif
