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

    // Run 4500 continuous simulation steps (90 seconds at 50Hz)
    int gameplayDemoSteps = 0;

    int deaths = 0;
    int cutsCompleted = 0;
    std::uint32_t lastClaimed = 0;
    std::uint32_t lastScore = 0;

    for (int i = 0; i < 4500; ++i) {
        const auto prevLives = game.getView().stats.lives;
        game.step(20);
        const auto& view = game.getView();
        if (view.attractStage == AttractStage::GameplayDemo) {
            ++gameplayDemoSteps;
            if (view.stats.lives < prevLives) {
                ++deaths;
            }
            if (view.stats.claimedCells > lastClaimed) {
                ++cutsCompleted;
                lastClaimed = view.stats.claimedCells;
            }
            lastScore = view.stats.score;
        }
    }
    EXPECT_GE(gameplayDemoSteps, 4000);
    EXPECT_EQ(deaths, 0);
    EXPECT_GE(cutsCompleted, 10);
    EXPECT_GT(lastScore, 1000U);
}

TEST(DemoBotTest, MultiSegmentRibbonDistance)
{
    DemoBot bot;
    GameView view {};

    std::deque<LineSegment> ribbon {};
    ribbon.push_back(LineSegment {Point {10, 10}, Point {20, 10}});
    ribbon.push_back(LineSegment {Point {20, 10}, Point {30, 20}});
    view.qixRibbons.push_back(ribbon);

    // Closest to second segment at (25, 15)
    EXPECT_NEAR(bot.getQixDistance(view, Point {25, 15}), 0.0f, 0.01f);
    // Closest to first segment at (15, 15) -> dist is 5
    EXPECT_NEAR(bot.getQixDistance(view, Point {15, 15}), 5.0f, 0.01f);
}

TEST(DemoBotTest, MinQixDistanceToTrail)
{
    DemoBot bot;
    GameView view {};

    std::deque<LineSegment> ribbon {};
    ribbon.push_back(LineSegment {Point {50, 30}, Point {50, 40}});
    view.qixRibbons.push_back(ribbon);

    std::vector<Point> trail = {{10, 35}, {30, 35}, {45, 35}};
    // Point (45, 35) is 5 units from segment (50, 30)-(50, 40)
    EXPECT_NEAR(bot.getMinQixDistanceToTrail(view, trail), 5.0f, 0.01f);
}

TEST(DemoBotTest, ControlledCutInitiation)
{
    Playfield pf(80, 60);
    pf.initBorders();

    DemoBot bot;
    GameView view {};
    view.playfield = &pf;
    view.mode = GameMode::Classic;
    view.markerPos = Point {40, 59};
    view.drawMode = DrawMode::None;

    std::deque<LineSegment> ribbon {};
    ribbon.push_back(LineSegment {Point {40, 10}, Point {45, 10}});
    view.qixRibbons.push_back(ribbon);

    view.sparxPositions = {Point {0, 0}, Point {79, 0}};

    bool initiatedCut = false;
    for (int i = 0; i < 10; ++i) {
        const auto cmd = bot.update(view);
        if (cmd.drawMode != DrawMode::None) {
            initiatedCut = true;
            break;
        }
    }
    EXPECT_TRUE(initiatedCut);
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
