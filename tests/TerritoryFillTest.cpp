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
    EXPECT_FALSE(result.splitOccurred);
}

TEST(TerritoryFillTest, SplitQixDetectsSeparationAndAwardsThreshold)
{
    // 10x10 field
    qix::Playfield field {10, 10};
    qix::TerritoryFill fill {10, 10};

    // Draw vertical dividing line along x = 5 from y = 0 to y = 9
    std::vector<qix::Point> trail {};
    for (std::int32_t y {0}; y < 10; ++y) {
        trail.push_back(qix::Point {5, y});
    }

    // Place Qix 1 on left side (x = 2, y = 5), Qix 2 on right side (x = 7, y = 5)
    std::vector<qix::Point> qixPositions {qix::Point {2, 5}, qix::Point {7, 5}};

    const auto result = fill.execute(field, trail, qixPositions, qix::DrawMode::Slow, 75);

    // Splitting the Qixes should trigger splitOccurred and complete the level immediately
    EXPECT_TRUE(result.splitOccurred);
    EXPECT_TRUE(result.thresholdMet);

    // Both compartments hold an active Qix, so neither side is claimed
    EXPECT_EQ(field.getCell(2, 5), qix::CellState::Empty);
    EXPECT_EQ(field.getCell(7, 5), qix::CellState::Empty);
}

TEST(TerritoryFillTest, TwoQixSameSideNoSplit)
{
    // 10x10 field
    qix::Playfield field {10, 10};
    qix::TerritoryFill fill {10, 10};

    // Trail at x = 3 from y = 0 to y = 9
    std::vector<qix::Point> trail {};
    for (std::int32_t y {0}; y < 10; ++y) {
        trail.push_back(qix::Point {3, y});
    }

    // Both Qixes on the right side
    std::vector<qix::Point> qixPositions {qix::Point {5, 5}, qix::Point {7, 5}};

    const auto result = fill.execute(field, trail, qixPositions, qix::DrawMode::Fast, 75);

    EXPECT_FALSE(result.splitOccurred);
    EXPECT_FALSE(result.thresholdMet);
    EXPECT_EQ(result.claimedCellsCount, 16U);

    // Left side cells are claimed
    EXPECT_EQ(field.getCell(1, 1), qix::CellState::ClaimedFast);
    // Right side cells remain empty
    EXPECT_EQ(field.getCell(5, 5), qix::CellState::Empty);
}

TEST(TerritoryFillTest, ScoreMultiplierApplied)
{
    qix::Playfield field {10, 10};
    qix::TerritoryFill fill {10, 10};

    std::vector<qix::Point> trail {};
    for (std::int32_t y {0}; y < 10; ++y) {
        trail.push_back(qix::Point {3, y});
    }

    std::vector<qix::Point> qixPositions {qix::Point {6, 5}};

    // Multiplier of 3x with Slow Draw (200 pts/cell * 3 = 600 pts/cell * 16 cells = 9600 pts)
    const auto result = fill.execute(field, trail, qixPositions, qix::DrawMode::Slow, 75, 3);
    EXPECT_EQ(result.claimedCellsCount, 16U);
    EXPECT_EQ(result.pointsAwarded, 16U * 200U * 3U);
    EXPECT_EQ(result.thresholdBonus, 0U);
}

TEST(TerritoryFillTest, ThresholdOvershootBonusCalculation)
{
    // 10x10 field: 64 playable interior cells
    qix::Playfield field {10, 10};
    qix::TerritoryFill fill {10, 10};

    // Partition at x = 8, claiming x = 1..7 (56 cells = 87% of 64)
    std::vector<qix::Point> trail {};
    for (std::int32_t y {0}; y < 10; ++y) {
        trail.push_back(qix::Point {8, y});
    }

    std::vector<qix::Point> qixPositions {qix::Point {9, 5}}; // Qix on right

    // Multiplier 2x, Fast Draw (100 pts/cell)
    // 87% claimed - 75% target = 12% overshoot
    // thresholdBonus = 12 * 1000 * 2 = 24000
    // base points = 56 * 100 * 2 = 11200
    // total = 35200
    const auto result = fill.execute(field, trail, qixPositions, qix::DrawMode::Fast, 75, 2);
    EXPECT_TRUE(result.thresholdMet);
    EXPECT_EQ(result.claimedPercent, 87U);
    EXPECT_EQ(result.thresholdBonus, 24000U);
    EXPECT_EQ(result.pointsAwarded, 11200U + 24000U);
}

TEST(TerritoryFillTest, ExactThresholdZeroBonus)
{
    // 10x10 field: 64 playable interior cells
    qix::Playfield field {10, 10};
    qix::TerritoryFill fill {10, 10};

    // Partition at x = 7, claiming x = 1..6 (48 cells = exactly 75% of 64)
    std::vector<qix::Point> trail {};
    for (std::int32_t y {0}; y < 10; ++y) {
        trail.push_back(qix::Point {7, y});
    }

    std::vector<qix::Point> qixPositions {qix::Point {8, 5}}; // Qix on right

    const auto result = fill.execute(field, trail, qixPositions, qix::DrawMode::Slow, 75, 1);
    EXPECT_TRUE(result.thresholdMet);
    EXPECT_EQ(result.claimedPercent, 75U);
    EXPECT_EQ(result.thresholdBonus, 0U);
    EXPECT_EQ(result.pointsAwarded, 48U * 200U * 1U);
}
