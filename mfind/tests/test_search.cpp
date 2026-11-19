#include "test_framework.h"
#include "../helpers/searchHelper.h"
#include <filesystem>
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
