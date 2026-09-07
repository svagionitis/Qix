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
