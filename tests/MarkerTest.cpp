#include "Marker.h"
#include <gtest/gtest.h>

TEST(MarkerTest, BorderNavigation)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {5, 0}, 3};

    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 0}));
    EXPECT_FALSE(marker.isDrawing());
    EXPECT_EQ(marker.getLives(), 3);

    // Navigate right along top border
    const bool movedRight = marker.move(field, qix::PlayerCommand {qix::Direction::Right, qix::DrawMode::None});
    EXPECT_TRUE(movedRight);
    EXPECT_EQ(marker.getPosition(), (qix::Point {6, 0}));

    // Cannot step into empty space without draw command
    const bool steppedIntoEmpty = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::None});
    EXPECT_FALSE(steppedIntoEmpty);
    EXPECT_EQ(marker.getPosition(), (qix::Point {6, 0}));
}

TEST(MarkerTest, DrawingStixIntoEmptySpace)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {5, 0}, 3};

    // Step into empty space with DrawMode::Slow
    const bool startDraw = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Slow});
    EXPECT_TRUE(startDraw);
    EXPECT_TRUE(marker.isDrawing());
    EXPECT_EQ(marker.getDrawMode(), qix::DrawMode::Slow);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 1}));
    EXPECT_EQ(field.getCell(5, 1), qix::CellState::ActiveStix);
    EXPECT_EQ(marker.getTrail().size(), 2U); // (5,0) and (5,1)

    // Cannot reverse onto itself
    const bool reverse = marker.move(field, qix::PlayerCommand {qix::Direction::Up, qix::DrawMode::Slow});
    EXPECT_FALSE(reverse);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 1}));
}

TEST(MarkerTest, LivesAccounting)
{
    qix::Marker marker {qix::Point {0, 0}, 2};
    EXPECT_TRUE(marker.isAlive());

    marker.decrementLives();
    EXPECT_EQ(marker.getLives(), 1);
    EXPECT_TRUE(marker.isAlive());

    marker.decrementLives();
    EXPECT_EQ(marker.getLives(), 0);
    EXPECT_FALSE(marker.isAlive());
}
