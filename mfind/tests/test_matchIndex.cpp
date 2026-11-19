#include "test_framework.h"
#include "../core/matchIndex.h"
#include <string>
#include <vector>

TEST(MatchIndex_AddAndRetrieveByFileAndKeyword) {
    MatchIndex idx;
    std::vector<Match> ms = {
        Match{"file1.txt", 10, "foo", "...foo..."},
        Match{"file1.txt", 20, "bar", "...bar..."}
    };
    idx.updateMatchesForFile("file1.txt", ms);

    EXPECT_EQ(idx.getMatchesForFile("file1.txt").size(), (size_t)2);
    EXPECT_EQ(idx.getMatchesForKeyword("foo").size(), (size_t)1);
    EXPECT_EQ(idx.getMatchesForKeyword("bar").size(), (size_t)1);
}

TEST(MatchIndex_UpdateEvictsStaleMatches) {
    MatchIndex idx;
    idx.updateMatchesForFile("f.txt", {Match{"f.txt", 5, "old", "old"}});
    idx.updateMatchesForFile("f.txt", {Match{"f.txt", 5, "new", "new"}});

    EXPECT_EQ(idx.getMatchesForKeyword("old").size(), (size_t)0);
    EXPECT_EQ(idx.getMatchesForKeyword("new").size(), (size_t)1);
}

TEST(MatchIndex_HasMatchesForFile) {
    MatchIndex idx;
    EXPECT_FALSE(idx.hasMatchesForFile("missing.txt"));
    idx.updateMatchesForFile("here.txt", {Match{"here.txt", 0, "x", "x"}});
    EXPECT_TRUE(idx.hasMatchesForFile("here.txt"));
}

TEST(MatchIndex_ClearRemovesAll) {
    MatchIndex idx;
    idx.updateMatchesForFile("f.txt", {Match{"f.txt", 0, "x", "x"}});
    idx.clear();
    EXPECT_FALSE(idx.hasMatchesForFile("f.txt"));
    EXPECT_EQ(idx.getMatchesForKeyword("x").size(), (size_t)0);
}

TEST(MatchIndex_EmptyKeywordReturnsEmpty) {
    MatchIndex idx;
    EXPECT_EQ(idx.getMatchesForKeyword("nonexistent").size(), (size_t)0);
}

TEST(MatchIndex_TwoFilesIndependent) {
    MatchIndex idx;
    idx.updateMatchesForFile("a.txt", {Match{"a.txt", 0, "foo", "foo"}});
    idx.updateMatchesForFile("b.txt", {Match{"b.txt", 0, "foo", "foo"}});
    EXPECT_EQ(idx.getMatchesForFile("a.txt").size(), (size_t)1);
    EXPECT_EQ(idx.getMatchesForFile("b.txt").size(), (size_t)1);
    EXPECT_EQ(idx.getMatchesForKeyword("foo").size(), (size_t)2);
}

TEST(MatchIndex_RepeatedUpdateRefreshesContextWithoutDuplicates) {
    MatchIndex idx;
    idx.updateMatchesForFile("f", {{"f", 0, "foo", "old"}});
    auto snapshot = idx.getMatchesForFile("f");
    idx.updateMatchesForFile("f", {{"f", 0, "foo", "new"}, {"f", 0, "foo", "new"}});
    EXPECT_EQ(idx.getMatchesForKeyword("foo").size(), size_t(1));
    EXPECT_EQ(idx.getMatchesForKeyword("foo")[0].context, std::string("new"));
    EXPECT_EQ(snapshot[0].context, std::string("old"));
    idx.removeFile("f"); EXPECT_FALSE(idx.hasMatchesForFile("f"));
    EXPECT_TRUE(idx.getMatchesForKeyword("foo").empty());
}
