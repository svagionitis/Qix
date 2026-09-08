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

TEST(MarkerTest, IncrementLivesAndCapAtMaximum)
{
    qix::Marker marker {qix::Point {0, 0}, 8};
    EXPECT_EQ(marker.getLives(), 8U);

    // Increment to 9 (MaxLives)
    marker.incrementLives();
    EXPECT_EQ(marker.getLives(), 9U);

    // Capped at MaxLives (9)
    marker.incrementLives();
    EXPECT_EQ(marker.getLives(), 9U);
}

TEST(MarkerTest, ClassicModeClaimedCellsImpassable)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {5, 0}, 3, qix::GameMode::Classic};

    // Mark neighbor cell as claimed territory
    field.setCell(5, 1, qix::CellState::ClaimedSlow);
    field.setCell(6, 0, qix::CellState::ClaimedFast);

    // In Classic mode, marker cannot step into ClaimedSlow
    const bool stepIntoSlow = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::None});
    EXPECT_FALSE(stepIntoSlow);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 0}));

    // In Classic mode, marker cannot step into ClaimedFast
    const bool stepIntoFast = marker.move(field, qix::PlayerCommand {qix::Direction::Right, qix::DrawMode::None});
    EXPECT_FALSE(stepIntoFast);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 0}));

    // But border navigation is permitted
    const bool stepIntoBorder = marker.move(field, qix::PlayerCommand {qix::Direction::Left, qix::DrawMode::None});
    EXPECT_TRUE(stepIntoBorder);
    EXPECT_EQ(marker.getPosition(), (qix::Point {4, 0}));
}

TEST(MarkerTest, ModernModeClaimedCellsWalkable)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {5, 0}, 3, qix::GameMode::Modern};

    // Mark neighbor cells as claimed territory
    field.setCell(5, 1, qix::CellState::ClaimedSlow);
    field.setCell(5, 2, qix::CellState::ClaimedFast);

    // In Modern mode, marker can step into ClaimedSlow
    const bool stepIntoSlow = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::None});
    EXPECT_TRUE(stepIntoSlow);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 1}));

    // In Modern mode, marker can also step into ClaimedFast
    const bool stepIntoFast = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::None});
    EXPECT_TRUE(stepIntoFast);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 2}));
}

TEST(MarkerTest, ReleasingDrawButtonHaltsMarkerOnStix)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {5, 0}, 3};

    // Begin drawing Slow into empty territory
    const bool startDraw = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Slow});
    EXPECT_TRUE(startDraw);
    EXPECT_TRUE(marker.isDrawing());
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 1}));

    // Releasing draw button (DrawMode::None) halts the marker mid-stroke
    const bool releasedMove = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::None});
    EXPECT_FALSE(releasedMove);
    EXPECT_TRUE(marker.isDrawing());
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 1}));

    // Re-pressing the matching draw button allows advancing again
    const bool resumedMove = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Slow});
    EXPECT_TRUE(resumedMove);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 2}));
}

TEST(MarkerTest, SwitchingDrawModeMidStrokeDisallowed)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {5, 0}, 3};

    // Begin drawing Slow into empty territory
    const bool startDraw = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Slow});
    EXPECT_TRUE(startDraw);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 1}));

    // Attempting to switch to Fast draw mid-stroke is rejected
    const bool switchMode = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Fast});
    EXPECT_FALSE(switchMode);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 1}));

    // Continuing with active Slow mode succeeds
    const bool continueSlow = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Slow});
    EXPECT_TRUE(continueSlow);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 2}));
}

TEST(MarkerTest, HoldingDrawModeAdvancesStix)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {5, 0}, 3};

    // Begin drawing Fast
    const bool startDraw = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Fast});
    EXPECT_TRUE(startDraw);
    EXPECT_EQ(marker.getDrawMode(), qix::DrawMode::Fast);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 1}));

    // Advance across empty cells while holding Fast
    for (std::int32_t y = 2; y < 19; ++y) {
        const bool advanced = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Fast});
        EXPECT_TRUE(advanced);
        EXPECT_EQ(marker.getPosition(), (qix::Point {5, y}));
    }

    // Connect to opposite border (y=19) while holding Fast
    const bool closedLoop = marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Fast});
    EXPECT_TRUE(closedLoop);
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 19}));
}

TEST(MarkerTest, FastVsSlowDrawPacing)
{
    qix::Playfield field {30, 30};
    qix::Marker fastMarker {qix::Point {5, 0}, 3};
    qix::Marker slowMarker {qix::Point {15, 0}, 3};

    // 10 ticks of Fast Draw
    std::int32_t fastSteps {0};
    for (std::int32_t i {0}; i < 10; ++i) {
        if (fastMarker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Fast})) {
            ++fastSteps;
        }
    }
    EXPECT_EQ(fastSteps, 10);
    EXPECT_EQ(fastMarker.getPosition().y, 10);

    // 10 ticks of Slow Draw
    std::int32_t slowSteps {0};
    for (std::int32_t i {0}; i < 10; ++i) {
        if (slowMarker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Slow})) {
            ++slowSteps;
        }
    }
    // In 10 ticks, Slow Draw takes 5 steps (half speed of Fast Draw)
    EXPECT_EQ(slowSteps, 5);
    EXPECT_EQ(slowMarker.getPosition().y, 5);
}
