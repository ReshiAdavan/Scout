#include "core/searchSession.h"
#include "helpers/cli.h"
#include "helpers/output.h"
#include <csignal>
#include <chrono>
#include <thread>
#include <unistd.h>
volatile std::sig_atomic_t running = 1;
void signalHandler(int) { running = 0; }
int main(int argc, char** argv) {
    try {
        const auto options = parseOptions(argc, argv);
        if (options.help) {
            std::cout << "Usage: mfind [root] [--keywords FILE] [--watch] [--threads N]\n"
                         "  --extension EXT (repeatable) --poll-ms N --color auto|always|never\n"
                         "--format text|editor|json|jsonl|csv\n"
                         "Exit codes: 0 matches, 1 no matches, 2 error; watch exits 0 on interrupt.\n";
            return 0;
        }
        if (options.version) { std::cout << "Scout 0.2\n"; return 0; }
        std::signal(SIGINT, signalHandler); std::signal(SIGTERM, signalHandler);
        SearchSession session;
        while (running) {
            const auto words = loadKeywords(options.keywords);
            const auto matches = session.scan(options, words);
            printResults(matches, options);
            if (!options.watch) return matches.empty() ? 1 : 0;
            for (size_t waited = 0; running && waited < options.pollMs; waited += 50)
                std::this_thread::sleep_for(std::chrono::milliseconds(std::min<size_t>(50, options.pollMs - waited)));
        }
        return 0;
    } catch (const std::exception& error) { std::cerr << "mfind: " << error.what() << '\n'; return 2; }
}
