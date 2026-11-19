#include "core/searchSession.h"
#include "helpers/cli.h"
#include "helpers/output.h"
#include "helpers/matchView.h"
#include "helpers/statistics.h"
#include "helpers/watchOutput.h"
#include <csignal>
#include <chrono>
#include <thread>
#include <unistd.h>
volatile std::sig_atomic_t running = 1;
void signalHandler(int) { running = 0; }
int main(int argc, char** argv) {
    try {
        auto options = parseOptions(argc, argv);
        if (options.help) {
            std::cout << "Usage: mfind [root] [--keywords FILE] [--watch] [--threads N]\n"
                         "  --cache FILE --index-mb N\n"
                         "  --stats --require-all ID (repeat) | --require-any ID (repeat)\n"
                         "  --config FILE --profile NAME\n"
                         "  --match-mode all|longest-per-start|leftmost-longest\n"
                         "  --ignore-case --whole-word\n"
                         "  --extension EXT (repeatable) --poll-ms N --color auto|always|never\n"
                         "--format text|editor|json|jsonl|csv|lsp\n"
                         "Exit codes: 0 matches, 1 no matches, 2 error; watch exits 0 on interrupt.\n";
            return 0;
        }
        if (options.version) { std::cout << "Scout 0.2\n"; return 0; }
        std::signal(SIGINT, signalHandler); std::signal(SIGTERM, signalHandler);
        SearchSession session;
        std::vector<Match> previous;
        while (running) {
            const auto started = std::chrono::steady_clock::now();
            options = parseOptions(argc, argv);
            std::vector<std::string> words;
            if (options.rules.empty()) words = loadKeywords(options.keywords);
            else { std::set<std::string> seen; for (const auto& rule : options.rules) if (seen.insert(rule.pattern).second) words.push_back(rule.pattern); }
            auto matches = selectMatches(session.scan(options, words), options.matchMode, words);
            if (!options.rules.empty()) {
                std::vector<Match> labeled;
                for (const auto& match : matches) for (const auto& rule : options.rules) if (rule.pattern == match.keyword) {
                    auto copy = match; copy.id = rule.id; copy.tag = rule.tag; labeled.push_back(std::move(copy));
                }
                matches = std::move(labeled);
            }
            matches = queryFiles(matches, options, words);
            if (options.watch) { printChanges(previous, matches, options); previous = matches; }
            else printResults(matches, options);
            if (options.stats) printStatistics(matches, options, words, session,
                std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count());
            if (!options.watch) return matches.empty() ? 1 : 0;
            for (size_t waited = 0; running && waited < options.pollMs; waited += 50)
                std::this_thread::sleep_for(std::chrono::milliseconds(std::min<size_t>(50, options.pollMs - waited)));
        }
        return 0;
    } catch (const std::exception& error) { std::cerr << "mfind: " << error.what() << '\n'; return 2; }
}
