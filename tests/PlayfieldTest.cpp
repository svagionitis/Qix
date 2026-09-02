#include "Playfield.h"
#include <gtest/gtest.h>

TEST(PlayfieldTest, DimensionsAndBoundaries)
{
    qix::Playfield field {20, 15};

    EXPECT_EQ(field.getWidth(), 20);
    EXPECT_EQ(field.getHeight(), 15);
    EXPECT_EQ(field.getInteriorCount(), (20 - 2) * (15 - 2));

    // Outer perimeter must be Border
    EXPECT_EQ(field.getCell(0, 0), qix::CellState::Border);
    EXPECT_EQ(field.getCell(19, 0), qix::CellState::Border);
    EXPECT_EQ(field.getCell(0, 14), qix::CellState::Border);
    EXPECT_EQ(field.getCell(19, 14), qix::CellState::Border);

    // Interior must initially be Empty
    EXPECT_EQ(field.getCell(1, 1), qix::CellState::Empty);
    EXPECT_EQ(field.getCell(10, 7), qix::CellState::Empty);
}

TEST(PlayfieldTest, BoundsChecking)
{
    qix::Playfield field {10, 10};

    EXPECT_TRUE(field.isInBounds(0, 0));
    EXPECT_TRUE(field.isInBounds(9, 9));
    EXPECT_FALSE(field.isInBounds(-1, 5));
    EXPECT_FALSE(field.isInBounds(10, 5));
    EXPECT_FALSE(field.isInBounds(5, -1));
    EXPECT_FALSE(field.isInBounds(5, 10));

    // Access out-of-bounds yields Border for safety
    EXPECT_EQ(field.getCell(-1, -1), qix::CellState::Border);
}

TEST(PlayfieldTest, CellManipulationAndClaimCount)
{
    qix::Playfield field {10, 10};

    field.setCell(2, 2, qix::CellState::ClaimedSlow);
    field.setCell(3, 3, qix::CellState::ClaimedFast);
    EXPECT_EQ(field.getCell(2, 2), qix::CellState::ClaimedSlow);
    EXPECT_EQ(field.getCell(3, 3), qix::CellState::ClaimedFast);

    field.updateClaimedCount();
    EXPECT_EQ(field.getClaimedCount(), 2U);
}
