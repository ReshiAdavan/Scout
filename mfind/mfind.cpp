#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include "../aho/ahoCorasick.h"
#include "../helpers/searchHelper.h"
#include "../helpers/formatHelper.h"
#include "../helpers/configHelper.h"
#include "walkers/walker.h"
#include "queues/fileQueue.h"
#include "workers/workerPool.h"

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <root_dir> <keyword1> [keyword2] ...\n";
        return 1;
    }

    std::string root = argv[1];

    AhoCorasick ac;
    for (int i = 2; i < argc; ++i) {
        ac.addKeyword(argv[i]);
    }
    ac.build();
    std::unordered_map<std::string, std::string> colorMap = buildColorMap(ac.getKeywords());

    FileQueue fileQueue;
    Walker walker(2);
    walker.setFileCallback([&](const std::string& file) {
        fileQueue.push(file);
    });

    WorkerPool pool(4, fileQueue, [&](const std::string& file) {
        searchFile(file, ac, [&](const std::string& file, size_t pos, const std::string& word, const std::string& context) {
            printMatch(file, pos, word, context, colorMap);
        });
    });

    walker.start(root);
    pool.start();

    walker.join();
    fileQueue.setFinished();
    pool.join();

    return 0;
}
