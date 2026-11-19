#include "test_framework.h"
#include "../helpers/searchHelper.h"
#include <filesystem>
#include "../core/fileCache.h"
#include <unistd.h>

TEST(Search_ChunkBoundariesMatchWholeBuffer) {
    char name[] = "/tmp/scout-search-XXXXXX";
    int fd = mkstemp(name); close(fd);
    struct Cleanup { const char* path; ~Cleanup() { std::filesystem::remove(path); } } cleanup{name};
    const std::string text = std::string(4093, 'x') + "abcdefabcdef" + std::string(8190, 'a') + "foo";
    { std::ofstream out(name); out << text; }
    AhoCorasick ac;
    for (auto word : {"abc", "abcdef", "bc", "aa", "foo"}) ac.addKeyword(word);
    ac.build();
    auto expected = ac.search(text); std::sort(expected.begin(), expected.end());
    for (size_t size : {size_t(1), size_t(8), size_t(4096), size_t(8192)}) {
        std::vector<std::pair<size_t, std::string>> actual;
        for (const auto& m : searchFile(name, ac, size)) actual.emplace_back(m.byteOffset, m.keyword);
        std::sort(actual.begin(), actual.end());
        EXPECT_TRUE(actual == expected);
    }
}
TEST(Scanner_SerialCallbacksAndZeroThreads) {
    AhoCorasick ac; ac.addKeyword("aa"); ac.build();
    const auto caller = std::this_thread::get_id();
    size_t count = 0;
    ChunkScanner::scan(std::string(10000, 'a'), ac, [&](size_t, const std::string&, const std::string&) {
        EXPECT_TRUE(std::this_thread::get_id() == caller); ++count;
    }, 0, 0);
    EXPECT_EQ(count, size_t(9999));
}
TEST(Scanner_EmptyPatterns) {
    AhoCorasick ac; ac.build(); size_t count = 0;
    ChunkScanner::scan("abc", ac, [&](size_t, const std::string&, const std::string&) { ++count; });
    EXPECT_EQ(count, size_t(0));
}

TEST(FileCache_ConcurrentReadersAndWriters) {
    char name[] = "/tmp/scout-cache-XXXXXX";
    int fd = mkstemp(name); close(fd);
    struct Cleanup { const char* path; ~Cleanup() { std::filesystem::remove(path); } } cleanup{name};
    FileCache cache;
    std::vector<std::thread> workers;
    for (int i = 0; i < 8; ++i) workers.emplace_back([&] {
        for (int j = 0; j < 1000; ++j) { cache.update(name); cache.isChanged(name); }
    });
    for (auto& worker : workers) worker.join();
    EXPECT_FALSE(cache.isChanged(name));
}

TEST(FileCache_DetectsSameTimestampContentChanges) {
    char name[] = "/tmp/scout-fingerprint-XXXXXX";
    int fd = mkstemp(name); close(fd);
    struct Cleanup { const char* path; ~Cleanup() { std::filesystem::remove(path); } } cleanup{name};
    { std::ofstream out(name); out << "foo"; }
    const auto time = std::filesystem::last_write_time(name);
    FileCache cache; cache.update(name);
    { std::ofstream out(name); out << "bar"; }
    std::filesystem::last_write_time(name, time);
    EXPECT_TRUE(cache.isChanged(name));
}
TEST(Ignore_ScopedRulesNegationAndPruning) {
    char name[] = "/tmp/scout-ignore-XXXXXX"; EXPECT_TRUE(mkdtemp(name));
    const std::filesystem::path root(name);
    struct Cleanup { std::filesystem::path path; ~Cleanup() { std::filesystem::remove_all(path); } } cleanup{root};
    std::filesystem::create_directories(root / "sub");
    std::filesystem::create_directories(root / "build" / "nested");
    { std::ofstream f(root / ".gitignore"); f << "build/\n*.log\n!important.log\n/root.txt\nspace name.txt\n"; }
    { std::ofstream f(root / "sub" / ".gitignore"); f << "secret.txt\n"; }
    for (auto file : {"root.txt", "space name.txt", "a.log", "important.log", "sub/root.txt", "sub/secret.txt", "build/nested/x.txt"}) { std::ofstream f(root / file); f << "x"; }
    auto files = collectFiles(root);
    auto contains = [&](const char* file) { return std::find(files.begin(), files.end(), (root / file).string()) != files.end(); };
    EXPECT_TRUE(contains("important.log")); EXPECT_TRUE(contains("sub/root.txt"));
    EXPECT_FALSE(contains("root.txt")); EXPECT_FALSE(contains("space name.txt"));
    EXPECT_FALSE(contains("sub/secret.txt")); EXPECT_FALSE(contains("build/nested/x.txt"));
    EXPECT_FALSE(contains("a.log"));
}

#include "../helpers/matchView.h"

TEST(MatchModes_NestedTouchingAndTies) {
    std::vector<Match> matches = {{"f",0,"ab",""},{"f",1,"bc",""},{"f",0,"abc",""},{"f",3,"ab",""}};
    std::vector<std::string> words = {"ab","bc","abc"};
    EXPECT_EQ(selectMatches(matches, "all", words).size(), size_t(4));
    auto longest = selectMatches(matches, "longest-per-start", words);
    EXPECT_EQ(longest.size(), size_t(3)); EXPECT_EQ(longest[0].keyword, std::string("abc"));
    auto left = selectMatches(matches, "leftmost-longest", words);
    EXPECT_EQ(left.size(), size_t(2)); EXPECT_EQ(left[1].byteOffset, size_t(3));
    auto tie = selectMatches({{"f",0,"FOO",""},{"f",0,"foo",""}}, "leftmost-longest", {"foo","FOO"});
    EXPECT_EQ(tie[0].keyword, std::string("foo"));
}
