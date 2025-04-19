#ifndef WORKER_POOL_H
#define WORKER_POOL_H

#include <thread>
#include <vector>
#include <functional>
#include <string>
#include "../queues/fileQueue.h"

class WorkerPool {
private:
    std::vector<std::thread> workers;
    int numThreads;
    FileQueue& fileQueue;
    std::function<void(const std::string&)> workerTask;

public:
    WorkerPool(int numThreads, FileQueue& fq, std::function<void(const std::string&)> task)
        : numThreads(numThreads), fileQueue(fq), workerTask(task) {}

    void start() {
        for (int i = 0; i < numThreads; ++i) {
            workers.emplace_back([this]() {
                std::string filepath;
                while (fileQueue.tryPop(filepath)) {
                    workerTask(filepath);
                }
            });
        }
    }

    void join() {
        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }
    }
};

#endif
