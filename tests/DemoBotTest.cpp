#include "DemoBot.h"
#include "GameConfig.h"
#include "QixGame.h"
#include <gtest/gtest.h>

namespace qix::test {

TEST(DemoBotTest, GetDistanceToBoundary)
{
    Playfield pf(80, 60);
    pf.initBorders();

    // From center of playfield (40, 30)
    const Point center {40, 30};

    // Up reaches y=0 (30 steps)
    EXPECT_EQ(DemoBot::getDistanceToBoundary(&pf, center, Direction::Up, GameMode::Classic), 30);
    // Down reaches y=59 (29 steps)
    EXPECT_EQ(DemoBot::getDistanceToBoundary(&pf, center, Direction::Down, GameMode::Classic), 29);
    // Left reaches x=0 (40 steps)
    EXPECT_EQ(DemoBot::getDistanceToBoundary(&pf, center, Direction::Left, GameMode::Classic), 40);
    // Right reaches x=79 (39 steps)
    EXPECT_EQ(DemoBot::getDistanceToBoundary(&pf, center, Direction::Right, GameMode::Classic), 39);

    // Invalid direction or null playfield
    EXPECT_EQ(DemoBot::getDistanceToBoundary(nullptr, center, Direction::Up, GameMode::Classic), 9999);
    EXPECT_EQ(DemoBot::getDistanceToBoundary(&pf, center, Direction::None, GameMode::Classic), 9999);
}

TEST(DemoBotTest, SafeExitAvoidsTrailSelfIntersection)
{
    Playfield pf(80, 60);
    pf.initBorders();

    DemoBot bot;
    GameView view {};
    view.playfield = &pf;
    view.mode = GameMode::Classic;

    // Marker moved Up from (40, 59) to (40, 56)
    const Point marker {40, 56};
    view.markerPos = marker;
    view.drawMode = DrawMode::Fast;
    view.stixTrail = {{40, 59}, {40, 58}, {40, 57}, {40, 56}};

    // Down points to (40, 57), which is in the trail!
    const auto safeDir = bot.findQuickestSafeExit(view, marker, view.stixTrail);
    EXPECT_NE(safeDir, Direction::Down);
    EXPECT_NE(safeDir, Direction::None);
    // Must be Up, Left, or Right
    EXPECT_TRUE(safeDir == Direction::Up || safeDir == Direction::Left || safeDir == Direction::Right);
}

TEST(DemoBotTest, QixDistanceCalculation)
{
    DemoBot bot;
    GameView view {};

    std::deque<LineSegment> ribbon {};
    ribbon.push_back(LineSegment {Point {40, 20}, Point {45, 20}});
    view.qixRibbons.push_back(ribbon);

    // Point exactly at start of head segment
    EXPECT_NEAR(bot.getQixDistance(view, Point {40, 20}), 0.0f, 0.01f);
    // Point 10 units away along Y axis
    EXPECT_NEAR(bot.getQixDistance(view, Point {40, 30}), 10.0f, 0.01f);
}

TEST(DemoBotTest, SustainedAutonomousSimulation)
{
    QixGame game(80, 60, 75, GameMode::Classic, 20);
    game.startAttractMode();

    // Fast-forward past title and instruction stages directly into GameplayDemo
    game.step(6500); // Transitions from TitleScores to Instructions
    game.step(6500); // Transitions from Instructions to GameplayDemo
    ASSERT_EQ(game.getView().attractStage, AttractStage::GameplayDemo);

    // Run 500 continuous simulation steps
    for (int i = 0; i < 500; ++i) {
        game.step(20);
        const auto& view = game.getView();
        EXPECT_TRUE(view.markerPos.x >= 0 && view.markerPos.x < 80);
        EXPECT_TRUE(view.markerPos.y >= 0 && view.markerPos.y < 60);
    }
}

TEST(DemoBotTest, DemoDurationConfiguration)
{
    QixGame game(80, 60, 75, GameMode::Classic, 30);

    // Default duration is 90 seconds (90000 ms)
    EXPECT_EQ(game.getDemoDurationMs(), 90000U);

    // Custom duration
    game.setDemoDurationMs(60000U);
    EXPECT_EQ(game.getDemoDurationMs(), 60000U);

    // Clamped minimum (5000 ms)
    game.setDemoDurationMs(1000U);
    EXPECT_EQ(game.getDemoDurationMs(), 5000U);
}

TEST(DemoBotTest, ParseDemoDurationCliFlags)
{
    EXPECT_EQ(GameConfig::parseDemoDurationFlag({"--demo-duration", "60"}), 60000U);
    EXPECT_EQ(GameConfig::parseDemoDurationFlag({"--demo-duration=45"}), 45000U);
    EXPECT_EQ(GameConfig::parseDemoDurationFlag({"--attract-duration", "120"}), 120000U);
    EXPECT_EQ(GameConfig::parseDemoDurationFlag({"--attract-duration=75"}), 75000U);

    // Default fallback (90s = 90000 ms)
    EXPECT_EQ(GameConfig::parseDemoDurationFlag({}), 90000U);
}

} // namespace qix::test
