#include "TerritoryFill.h"
#include <gtest/gtest.h>

TEST(TerritoryFillTest, VerticalPartitionAndCapture)
{
    // 10x10 field has 8x8 = 64 interior empty cells
    qix::Playfield field {10, 10};
    qix::TerritoryFill fill {10, 10};

    // Draw vertical trail cutting field at x = 3 from y = 0 to y = 9
    std::vector<qix::Point> trail {};
    for (std::int32_t y {0}; y < 10; ++y) {
        trail.push_back(qix::Point {3, y});
    }

    // Place Qix in the right region (x = 6, y = 5)
    std::vector<qix::Point> qixPositions {qix::Point {6, 5}};

    // Left side is x in [1, 2], y in [1, 8] -> 2 columns * 8 rows = 16 cells
    // The trail itself at x = 3 becomes Border.
    const auto result = fill.execute(field, trail, qixPositions, qix::DrawMode::Slow, 75);

    EXPECT_EQ(result.claimedCellsCount, 16U);
    EXPECT_EQ(result.pointsAwarded, 16U * 200U); // Slow draw awards 200 pts/cell
    EXPECT_FALSE(result.thresholdMet);

    // Verify left cells are ClaimedSlow
    for (std::int32_t y {1}; y <= 8; ++y) {
        for (std::int32_t x {1}; x <= 2; ++x) {
            EXPECT_EQ(field.getCell(x, y), qix::CellState::ClaimedSlow);
        }
    }

    // Verify right region where Qix resides remains Empty
    EXPECT_EQ(field.getCell(6, 5), qix::CellState::Empty);
    EXPECT_EQ(field.getCell(4, 1), qix::CellState::Empty);

    // Trail at x=3 is now Border
    EXPECT_EQ(field.getCell(3, 1), qix::CellState::Border);
}

TEST(TerritoryFillTest, VictoryThresholdMet)
{
    // Small 6x6 field -> 4x4 = 16 interior cells
    qix::Playfield field {6, 6};
    qix::TerritoryFill fill {6, 6};

    // Draw line at x = 4 from y = 0 to y = 5
    // Left side has x in [1, 2, 3], y in [1, 2, 3, 4] -> 3 cols * 4 rows = 12 cells
    // 12 / 16 = 75%
    std::vector<qix::Point> trail {};
    for (std::int32_t y {0}; y < 6; ++y) {
        trail.push_back(qix::Point {4, y});
    }

    // Qix placed on the right column at x = 5 (wait, x=5 is border; so x=4 was trail, x=5 is border).
    // Wait: for 6x6, x=0 is border, x=5 is border. Interior x is 1,2,3,4.
    // If Qix is at (1, 1), right side at x=2,3,4 will be captured!
    std::vector<qix::Point> qixPositions {qix::Point {1, 1}};

    // Trail at x = 2
    trail.clear();
    for (std::int32_t y {0}; y < 6; ++y) {
        trail.push_back(qix::Point {2, y});
    }

    // Qix is at (1, 1) -> left column x=1 (4 cells) remains empty.
    // Right columns x=3, 4 (2 cols * 4 rows = 8 cells) are claimed.
    // 8 / 16 = 50%
    const auto result = fill.execute(field, trail, qixPositions, qix::DrawMode::Fast, 50);
    EXPECT_EQ(result.claimedCellsCount, 8U);
    EXPECT_EQ(result.claimedPercent, 50U);
    EXPECT_TRUE(result.thresholdMet);
}
