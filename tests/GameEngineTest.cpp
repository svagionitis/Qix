#include "QixGame.h"
#include <gtest/gtest.h>

TEST(GameEngineTest, InitialState)
{
    qix::QixGame game {40, 30, 75};
    const auto& view = game.getView();

    EXPECT_EQ(view.state, qix::GameState::Ready);
    EXPECT_EQ(view.stats.score, 0U);
    EXPECT_EQ(view.stats.claimedPercent, 0U);
    EXPECT_EQ(view.stats.lives, 3U);
    EXPECT_EQ(view.stats.level, 1U);
}

TEST(GameEngineTest, TransitionToPlayingOnInput)
{
    qix::QixGame game {40, 30, 75};

    game.handleInput(qix::PlayerCommand {qix::Direction::Right, qix::DrawMode::None});
    game.step(16);

    const auto& view = game.getView();
    EXPECT_EQ(view.state, qix::GameState::Playing);
}

TEST(GameEngineTest, CompleteDrawLoopAndScore)
{
    // Standard playfield 80x60 with Qix centered at (40, 30)
    qix::QixGame game {80, 60, 50};

    // Move left along bottom border from (40, 59) to (2, 59)
    for (std::int32_t i {0}; i < 38; ++i) {
        game.handleInput(qix::PlayerCommand {qix::Direction::Left, qix::DrawMode::None});
        game.step(16);
    }

    // Now at (2, 59). Draw upward along x=2 to top border (2, 0), far away from Qix center
    for (std::int32_t y {59}; y >= 0; --y) {
        game.handleInput(qix::PlayerCommand {qix::Direction::Up, qix::DrawMode::Slow});
        game.step(16);
    }

    const auto& view = game.getView();
    // One region of the field should now be claimed, score should be > 0
    EXPECT_GT(view.stats.claimedCells, 0U);
    EXPECT_GT(view.stats.score, 0U);
    EXPECT_GT(view.stats.claimedPercent, 0U);
}

TEST(GameEngineTest, ResetGameSession)
{
    qix::QixGame game {30, 30, 75};
    game.handleInput(qix::PlayerCommand {qix::Direction::Left, qix::DrawMode::None});
    game.step(16);

    game.reset();
    const auto& view = game.getView();
    EXPECT_EQ(view.state, qix::GameState::Ready);
    EXPECT_EQ(view.stats.score, 0U);
    EXPECT_EQ(view.stats.lives, 3U);
}

TEST(GameEngineTest, ModeConfiguration)
{
    qix::QixGame classicGame {40, 30, 75, qix::GameMode::Classic};
    EXPECT_EQ(classicGame.getGameMode(), qix::GameMode::Classic);
    EXPECT_EQ(classicGame.getView().mode, qix::GameMode::Classic);
    EXPECT_EQ(classicGame.getView().stats.mode, qix::GameMode::Classic);

    qix::QixGame modernGame {40, 30, 75, qix::GameMode::Modern};
    EXPECT_EQ(modernGame.getGameMode(), qix::GameMode::Modern);
    EXPECT_EQ(modernGame.getView().mode, qix::GameMode::Modern);
    EXPECT_EQ(modernGame.getView().stats.mode, qix::GameMode::Modern);
}

TEST(GameEngineTest, MultiplierDefaultsAndCarriesOverLevels)
{
    qix::QixGame game {40, 30, 75};
    const auto& view = game.getView();
    EXPECT_EQ(view.stats.multiplier, 1U);
    EXPECT_FALSE(view.stats.splitBonus);

    // Advancing level preserves multiplier
    game.nextLevel();
    EXPECT_EQ(game.getView().stats.level, 2U);
    EXPECT_EQ(game.getView().stats.multiplier, 1U);
    EXPECT_FALSE(game.getView().stats.splitBonus);

    // Reset restores multiplier to 1
    game.reset();
    EXPECT_EQ(game.getView().stats.level, 1U);
    EXPECT_EQ(game.getView().stats.multiplier, 1U);
    EXPECT_FALSE(game.getView().stats.splitBonus);
}

TEST(GameEngineTest, CountdownTimerTicksWhenPlaying)
{
    qix::QixGame game {40, 30, 75};
    const auto& view = game.getView();

    EXPECT_EQ(view.stats.timeRemainingMs, 60000U);
    EXPECT_EQ(view.stats.totalLevelTimeMs, 60000U);
    EXPECT_FALSE(view.stats.timeUp);

    // Timer does not tick down while state is Ready
    game.step(100);
    EXPECT_EQ(game.getView().stats.timeRemainingMs, 60000U);

    // Transition to Playing
    game.handleInput(qix::PlayerCommand {qix::Direction::Right, qix::DrawMode::None});
    game.step(100);
    EXPECT_EQ(game.getView().state, qix::GameState::Playing);
    EXPECT_EQ(game.getView().stats.timeRemainingMs, 59900U);
}

TEST(GameEngineTest, CountdownTimerLevelProgressionAndReset)
{
    EXPECT_EQ(qix::QixGame::computeLevelTimeMs(1), 60000U);
    EXPECT_EQ(qix::QixGame::computeLevelTimeMs(2), 55000U);
    EXPECT_EQ(qix::QixGame::computeLevelTimeMs(3), 50000U);
    EXPECT_EQ(qix::QixGame::computeLevelTimeMs(8), 30000U); // Clamps to 30s min

    qix::QixGame game {40, 30, 75};
    game.nextLevel();

    EXPECT_EQ(game.getView().stats.level, 2U);
    EXPECT_EQ(game.getView().stats.timeRemainingMs, 55000U);
    EXPECT_EQ(game.getView().stats.totalLevelTimeMs, 55000U);

    game.reset();
    EXPECT_EQ(game.getView().stats.level, 1U);
    EXPECT_EQ(game.getView().stats.timeRemainingMs, 60000U);
}

TEST(GameEngineTest, CountdownTimerExpirySpawnsEscalationSparx)
{
    qix::QixGame game {40, 30, 75};

    // Transition to playing
    game.handleInput(qix::PlayerCommand {qix::Direction::Left, qix::DrawMode::None});
    game.step(16);
    EXPECT_EQ(game.getView().sparxPositions.size(), 2U);

    // Advance time past the 60s budget
    game.step(60000);
    const auto& view = game.getView();

    EXPECT_TRUE(view.stats.timeUp);
    EXPECT_EQ(view.stats.timeRemainingMs, 15000U);
    EXPECT_GT(view.sparxPositions.size(), 2U);
}

TEST(GameEngineTest, AutomaticSpeedEscalationAcrossLevels)
{
    qix::QixGame game {40, 30, 75, qix::DefaultGameMode, 75U};

    // Level 1 starts at base delay
    EXPECT_EQ(game.getCurrentDelayMs(), 75U);
    EXPECT_EQ(game.getView().stats.currentDelayMs, 75U);

    // Level 2 escalates by 5ms
    game.nextLevel();
    EXPECT_EQ(game.getView().stats.level, 2U);
    EXPECT_EQ(game.getCurrentDelayMs(), 70U);
    EXPECT_EQ(game.getView().stats.currentDelayMs, 70U);

    // Level 3 escalates by another 5ms
    game.nextLevel();
    EXPECT_EQ(game.getView().stats.level, 3U);
    EXPECT_EQ(game.getCurrentDelayMs(), 65U);
    EXPECT_EQ(game.getView().stats.currentDelayMs, 65U);

    // Reset restores base delay
    game.reset();
    EXPECT_EQ(game.getView().stats.level, 1U);
    EXPECT_EQ(game.getCurrentDelayMs(), 75U);
    EXPECT_EQ(game.getView().stats.currentDelayMs, 75U);
}

TEST(GameEngineTest, ManualBaseSpeedAdjustmentAndLevelScaling)
{
    qix::QixGame game {40, 30, 75, qix::DefaultGameMode, 75U};

    // User adjusts baseline to 50ms at runtime
    game.setBaseDelayMs(50U);
    EXPECT_EQ(game.getCurrentDelayMs(), 50U);
    EXPECT_EQ(game.getView().stats.currentDelayMs, 50U);

    // Next level escalates from new base (50ms - 5ms = 45ms)
    game.nextLevel();
    EXPECT_EQ(game.getView().stats.level, 2U);
    EXPECT_EQ(game.getCurrentDelayMs(), 45U);
    EXPECT_EQ(game.getView().stats.currentDelayMs, 45U);
}

TEST(GameEngineTest, FuseLimitEscalatesWithLevel)
{
    EXPECT_EQ(qix::QixGame::computeFuseLimit(1), 25U);
    EXPECT_EQ(qix::QixGame::computeFuseLimit(2), 23U);
    EXPECT_EQ(qix::QixGame::computeFuseLimit(3), 21U);
    EXPECT_EQ(qix::QixGame::computeFuseLimit(10), 10U); // Clamped at 10 minimum
}

TEST(GameEngineTest, LevelCompleteAwardsThresholdOvershootBonus)
{
    // 80x60 field, targetPercent = 0 so claiming any territory triggers completion with overshoot
    qix::QixGame game {80, 60, 0};

    // Move left along bottom border from (40, 59) to (2, 59)
    for (std::int32_t i {0}; i < 38; ++i) {
        game.handleInput(qix::PlayerCommand {qix::Direction::Left, qix::DrawMode::None});
        game.step(16);
    }

    // Draw up along x=2 to top border (2, 0) in Slow draw mode
    for (std::int32_t y {59}; y >= 0; --y) {
        game.handleInput(qix::PlayerCommand {qix::Direction::Up, qix::DrawMode::Slow});
        game.step(16);
    }

    const auto& view = game.getView();
    EXPECT_EQ(view.state, qix::GameState::LevelComplete);
    EXPECT_EQ(view.stats.claimedCells, 58U);
    EXPECT_EQ(view.stats.claimedPercent, 1U);
    // 1% claimed - 0% target = 1% overshoot * 1000 pts * 1x multiplier = 1000 pts bonus
    EXPECT_EQ(view.stats.thresholdBonus, 1000U);
    // 58 cells * 200 pts (Slow) + 1000 bonus = 12600 pts
    EXPECT_EQ(view.stats.score, 12600U);

    // Advancing level clears the threshold bonus
    game.nextLevel();
    EXPECT_EQ(game.getView().stats.level, 2U);
    EXPECT_EQ(game.getView().stats.thresholdBonus, 0U);
}

TEST(GameEngineTest, Level3SpawnsSuperSparx)
{
    qix::QixGame game {40, 30, 75};
    // Level 1: Regular Sparx
    for (const auto& sp : game.getView().sparxList) {
        EXPECT_FALSE(sp.isSuper);
    }

    // Level 2: Regular Sparx
    game.nextLevel();
    for (const auto& sp : game.getView().sparxList) {
        EXPECT_FALSE(sp.isSuper);
    }

    // Level 3: Super Sparx
    game.nextLevel();
    EXPECT_EQ(game.getView().stats.level, 3U);
    ASSERT_EQ(game.getView().sparxList.size(), 2U);
    for (const auto& sp : game.getView().sparxList) {
        EXPECT_TRUE(sp.isSuper);
    }
}

TEST(GameEngineTest, TimerEscalationSpawnsSuperSparx)
{
    qix::QixGame game {40, 30, 75};
    // Start playing so timer ticks
    game.handleInput(qix::PlayerCommand {qix::Direction::Left, qix::DrawMode::None});
    game.step(16);

    // Level 1 initial 2 Sparx are regular
    ASSERT_EQ(game.getView().sparxList.size(), 2U);
    EXPECT_FALSE(game.getView().sparxList[0].isSuper);
    EXPECT_FALSE(game.getView().sparxList[1].isSuper);

    // Fast-forward countdown timer past 60s
    game.step(60000U);

    // Time up, additional Sparx spawned should be Super Sparx
    const auto& view = game.getView();
    EXPECT_TRUE(view.stats.timeUp);
    EXPECT_GT(view.sparxList.size(), 2U);
    EXPECT_TRUE(view.sparxList[2].isSuper);
}

TEST(GameEngineTest, ExtraLifeAwardedAtScoreMilestone)
{
    // 200x60 field: Qix centered at cx = 100, cy = 30
    qix::QixGame game {200, 60, 75};
    EXPECT_EQ(game.getView().stats.lives, 3U);
    EXPECT_EQ(game.getView().stats.nextExtraLifeScore, 50000U);

    // Move left along bottom border from (100, 59) to (6, 59) (94 steps)
    for (std::int32_t i {0}; i < 94; ++i) {
        game.handleInput(qix::PlayerCommand {qix::Direction::Left, qix::DrawMode::None});
        game.step(16);
    }

    // Draw up along x=6 to top border (6, 0) in Slow draw mode
    // Encloses x=1..5 (5 columns * 58 rows = 290 cells * 200 pts = 58,000 pts)
    for (std::int32_t y {59}; y >= 0; --y) {
        game.handleInput(qix::PlayerCommand {qix::Direction::Up, qix::DrawMode::Slow});
        game.step(16);
    }

    const auto& view = game.getView();
    EXPECT_GE(view.stats.score, 50000U);
    // Extra life awarded: 3 -> 4 lives
    EXPECT_EQ(view.stats.lives, 4U);
    // Next milestone advanced to 100,000
    EXPECT_EQ(view.stats.nextExtraLifeScore, 100000U);

    // Advancing level preserves extra lives and milestone
    game.nextLevel();
    EXPECT_EQ(game.getView().stats.level, 2U);
    EXPECT_EQ(game.getView().stats.lives, 4U);
    EXPECT_EQ(game.getView().stats.nextExtraLifeScore, 100000U);

    // Reset restores initial session state (3 lives, 50,000 milestone)
    game.reset();
    EXPECT_EQ(game.getView().stats.lives, 3U);
    EXPECT_EQ(game.getView().stats.nextExtraLifeScore, 50000U);
}
