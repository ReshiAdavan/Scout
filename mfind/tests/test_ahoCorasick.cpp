#include "test_framework.h"
#include "../core/ahoCorasick.h"
#include <algorithm>
#include <set>
#include <string>

TEST(AhoCorasick_SingleKeywordSingleMatch) {
    AhoCorasick ac;
    ac.addKeyword("foo");
    ac.build();
    auto results = ac.search("xfoox");
    EXPECT_EQ(results.size(), (size_t)1);
    EXPECT_EQ(results[0].first, (size_t)1);
    EXPECT_EQ(results[0].second, std::string("foo"));
}

TEST(AhoCorasick_NoMatch) {
    AhoCorasick ac;
    ac.addKeyword("xyz");
    ac.build();
    auto results = ac.search("abc");
    EXPECT_EQ(results.size(), (size_t)0);
}

TEST(AhoCorasick_OverlappingMatchesSameKeyword) {
    AhoCorasick ac;
    ac.addKeyword("aba");
    ac.build();
    // "abababa" contains "aba" at positions 0, 2, 4
    auto results = ac.search("abababa");
    EXPECT_EQ(results.size(), (size_t)3);
    std::set<size_t> positions;
    for (auto& [pos, _] : results) positions.insert(pos);
    EXPECT_TRUE(positions.count(0) && positions.count(2) && positions.count(4));
}

TEST(AhoCorasick_FailLinkMergesOutput) {
    // Classic Aho-Corasick example: {he, she, hers, his} on "ushers"
    AhoCorasick ac;
    ac.addKeyword("he");
    ac.addKeyword("she");
    ac.addKeyword("hers");
    ac.addKeyword("his");
    ac.build();
    auto results = ac.search("ushers");
    // Expected: she@1, he@2, hers@2
    EXPECT_EQ(results.size(), (size_t)3);
    std::set<std::pair<size_t, std::string>> got(results.begin(), results.end());
    EXPECT_TRUE(got.count({1, "she"}));
    EXPECT_TRUE(got.count({2, "he"}));
    EXPECT_TRUE(got.count({2, "hers"}));
}

TEST(AhoCorasick_AllMatchesAtSamePosition) {
    // {ab, abc, abcd} on "abcd" should emit all three since each ends at a
    // different position; current "all matches" semantics keep them all.
    AhoCorasick ac;
    ac.addKeyword("ab");
    ac.addKeyword("abc");
    ac.addKeyword("abcd");
    ac.build();
    auto results = ac.search("abcd");
    EXPECT_EQ(results.size(), (size_t)3);
}

TEST(AhoCorasick_MaxPatternLength) {
    AhoCorasick ac;
    ac.addKeyword("a");
    ac.addKeyword("hello");
    ac.addKeyword("hi");
    EXPECT_EQ(ac.getMaxPatternLength(), (size_t)5);
}

TEST(AhoCorasick_GetKeywordsReturnsInsertionOrder) {
    AhoCorasick ac;
    ac.addKeyword("first");
    ac.addKeyword("second");
    ac.addKeyword("third");
    const auto& kws = ac.getKeywords();
    EXPECT_EQ(kws.size(), (size_t)3);
    EXPECT_EQ(kws[0], std::string("first"));
    EXPECT_EQ(kws[2], std::string("third"));
}

TEST(AhoCorasick_RebuildIsIdempotent) {
    AhoCorasick ac; ac.addKeyword("he"); ac.addKeyword("she"); ac.build();
    auto expected = ac.search("she");
    for (int i = 0; i < 10; ++i) { ac.build(); EXPECT_TRUE(ac.search("she") == expected); }
    ac.addKeyword("hers"); ac.build(); EXPECT_EQ(ac.search("ushers").size(), size_t(3));
}

TEST(AhoCorasick_CaseInsensitivePreservesPatternIdentity) {
    AhoCorasick ac(true); ac.addKeyword("Foo"); ac.addKeyword("FOO"); ac.build();
    auto matches = ac.search("xfoo FOO");
    EXPECT_EQ(matches.size(), size_t(4));
    EXPECT_EQ(matches[0].first, size_t(1)); EXPECT_EQ(matches[0].second, std::string("Foo"));
}
