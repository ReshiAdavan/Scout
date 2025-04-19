#ifndef FILE_QUEUE_H
#define FILE_QUEUE_H

#include <queue>
#include <string>
#include <mutex>
#include <condition_variable>
#include <atomic>

class FileQueue {
private:
    std::queue<std::string> queue;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> finished = false;

public:
    void push(const std::string& filepath) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            queue.push(filepath);
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

#endif
