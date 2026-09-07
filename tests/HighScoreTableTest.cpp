#include "HighScoreTable.h"
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>

namespace qix::test {

TEST(HighScoreTableTest, DefaultTableInitialization)
{
    const HighScoreTable table {10};
    EXPECT_EQ(table.size(), 10U);
    EXPECT_EQ(table.capacity(), 10U);

    const auto& entries = table.getEntries();
    ASSERT_EQ(entries.size(), 10U);

    EXPECT_EQ(entries[0].initials, "QIX");
    EXPECT_EQ(entries[0].score, 50000U);
    EXPECT_EQ(table.getHighScore(), 50000U);

    // Verify descending order
    for (std::size_t i {1}; i < entries.size(); ++i) {
        EXPECT_GE(entries[i - 1].score, entries[i].score);
    }
}

TEST(HighScoreTableTest, QualifiesLogic)
{
    const HighScoreTable table {5};
    // Initial 5th default is STV with 15000
    EXPECT_FALSE(table.qualifies(0U));
    EXPECT_FALSE(table.qualifies(10000U));
    EXPECT_FALSE(table.qualifies(15000U));
    EXPECT_TRUE(table.qualifies(15001U));
    EXPECT_TRUE(table.qualifies(60000U));
}

TEST(HighScoreTableTest, GetRankCalculation)
{
    const HighScoreTable table {5};
    // Defaults for cap 5: QIX (50k), TAI (40k), TOA (30k), ARC (20k), STV (15k)
    EXPECT_EQ(table.getRank(60000U), 1U);
    EXPECT_EQ(table.getRank(45000U), 2U);
    EXPECT_EQ(table.getRank(35000U), 3U);
    EXPECT_EQ(table.getRank(25000U), 4U);
    EXPECT_EQ(table.getRank(16000U), 5U);
    EXPECT_EQ(table.getRank(10000U), 0U);
}

TEST(HighScoreTableTest, InsertAndTruncateCapacity)
{
    HighScoreTable table {3};
    // Defaults: QIX (50k), TAI (40k), TOA (30k)
    ASSERT_EQ(table.size(), 3U);

    EXPECT_TRUE(table.insert("NEW", 45000U, 4, GameMode::Classic));
    EXPECT_EQ(table.size(), 3U);
    EXPECT_EQ(table.getEntries()[1].initials, "NEW");
    EXPECT_EQ(table.getEntries()[1].score, 45000U);
    EXPECT_EQ(table.getEntries()[2].initials, "TAI");

    // Attempt insert of non-qualifying score
    EXPECT_FALSE(table.insert("LOW", 1000U, 1, GameMode::Classic));
}

TEST(HighScoreTableTest, SanitizeInitials)
{
    EXPECT_EQ(HighScoreTable::sanitizeInitials("abc"), "ABC");
    EXPECT_EQ(HighScoreTable::sanitizeInitials("a!"), "A!A");
    EXPECT_EQ(HighScoreTable::sanitizeInitials("LongName"), "LON");
    EXPECT_EQ(HighScoreTable::sanitizeInitials(""), "AAA");
}

TEST(HighScoreTableTest, FileSaveAndLoadRoundtrip)
{
    const std::string testPath {"test_highscores_roundtrip.tmp"};
    HighScoreTable original {5};

    EXPECT_TRUE(original.insert("TST", 99999U, 9, GameMode::Modern));
    EXPECT_TRUE(original.saveToFile(testPath));

    HighScoreTable loaded {5};
    EXPECT_TRUE(loaded.loadFromFile(testPath));

    ASSERT_EQ(loaded.size(), 5U);
    EXPECT_EQ(loaded.getHighScore(), 99999U);
    EXPECT_EQ(loaded.getEntries().front().initials, "TST");
    EXPECT_EQ(loaded.getEntries().front().mode, GameMode::Modern);

    std::remove(testPath.c_str());
}

TEST(HighScoreTableTest, CorruptFileHandling)
{
    const std::string corruptPath {"test_highscores_corrupt.tmp"};
    std::ofstream file {corruptPath};
    file << "# Header only with garbage\n;;; bad data\n";
    file.close();

    HighScoreTable table {5};
    EXPECT_FALSE(table.loadFromFile(corruptPath));

    std::remove(corruptPath.c_str());
}

} // namespace qix::test
