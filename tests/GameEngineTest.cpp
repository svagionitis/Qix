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
