#include "TuiRenderer.h"
#include <gtest/gtest.h>

using namespace qix;
using namespace qix::tui;

TEST(TuiInputTest, SingleArrowKeys)
{
    TuiRenderer renderer {};
    TuiAction action {TuiAction::None};

    auto cmd = renderer.processInput("\033[A", action);
    EXPECT_EQ(cmd.direction, Direction::Up);
    EXPECT_EQ(action, TuiAction::None);

    cmd = renderer.processInput("\033[B", action);
    EXPECT_EQ(cmd.direction, Direction::Down);
    EXPECT_EQ(action, TuiAction::None);

    cmd = renderer.processInput("\033[C", action);
    EXPECT_EQ(cmd.direction, Direction::Right);
    EXPECT_EQ(action, TuiAction::None);

    cmd = renderer.processInput("\033[D", action);
    EXPECT_EQ(cmd.direction, Direction::Left);
    EXPECT_EQ(action, TuiAction::None);
}

TEST(TuiInputTest, HoldingArrowKeysMultiRepeat)
{
    TuiRenderer renderer {};
    TuiAction action {TuiAction::None};

    // 3 consecutive arrow right sequences: \033[C\033[C\033[C
    const auto cmd = renderer.processInput("\033[C\033[C\033[C", action);
    EXPECT_EQ(cmd.direction, Direction::Right);
    EXPECT_EQ(action, TuiAction::None);
    EXPECT_NE(action, TuiAction::SpeedDown);
}

TEST(TuiInputTest, EscapeSequenceSplittingAcrossChunks)
{
    TuiRenderer renderer {};
    TuiAction action {TuiAction::None};

    // Chunk 1: prefix "\033["
    auto cmd = renderer.processInput("\033[", action);
    EXPECT_EQ(cmd.direction, Direction::None);
    EXPECT_EQ(action, TuiAction::None);

    // Chunk 2: suffix "C" completes the sequence
    cmd = renderer.processInput("C", action);
    EXPECT_EQ(cmd.direction, Direction::Right);
    EXPECT_EQ(action, TuiAction::None);
    EXPECT_NE(action, TuiAction::SpeedDown);
}

TEST(TuiInputTest, FragmentBufferChopReproducesOldBugAndPasses)
{
    TuiRenderer renderer {};
    TuiAction action {TuiAction::None};

    // Chunk 1: exactly 7 bytes of a 9-byte stream "\033[C\033[C\033"
    auto cmd = renderer.processInput("\033[C\033[C\033", action);
    EXPECT_EQ(cmd.direction, Direction::Right);
    EXPECT_EQ(action, TuiAction::None);

    // Chunk 2: remaining 2 bytes "[C"
    cmd = renderer.processInput("[C", action);
    EXPECT_EQ(cmd.direction, Direction::Right);
    // Crucial check: must NOT be interpreted as '[' (SpeedDown)
    EXPECT_EQ(action, TuiAction::None);
    EXPECT_NE(action, TuiAction::SpeedDown);
}

TEST(TuiInputTest, WASDMovement)
{
    TuiRenderer renderer {};
    TuiAction action {TuiAction::None};

    EXPECT_EQ(renderer.processInput("w", action).direction, Direction::Up);
    EXPECT_EQ(renderer.processInput("s", action).direction, Direction::Down);
    EXPECT_EQ(renderer.processInput("a", action).direction, Direction::Left);
    EXPECT_EQ(renderer.processInput("d", action).direction, Direction::Right);

    EXPECT_EQ(renderer.processInput("W", action).direction, Direction::Up);
    EXPECT_EQ(renderer.processInput("S", action).direction, Direction::Down);
    EXPECT_EQ(renderer.processInput("A", action).direction, Direction::Left);
    EXPECT_EQ(renderer.processInput("D", action).direction, Direction::Right);
}

TEST(TuiInputTest, SpeedControlsDoNotUseBrackets)
{
    TuiRenderer renderer {};
    TuiAction action {TuiAction::None};

    (void)renderer.processInput("-", action);
    EXPECT_EQ(action, TuiAction::SpeedDown);

    (void)renderer.processInput("+", action);
    EXPECT_EQ(action, TuiAction::SpeedUp);

    (void)renderer.processInput("_", action);
    EXPECT_EQ(action, TuiAction::SpeedDown);

    (void)renderer.processInput("=", action);
    EXPECT_EQ(action, TuiAction::SpeedUp);

    // Standalone brackets should NOT adjust speed
    (void)renderer.processInput("[", action);
    EXPECT_NE(action, TuiAction::SpeedDown);

    (void)renderer.processInput("]", action);
    EXPECT_NE(action, TuiAction::SpeedUp);
}

TEST(TuiInputTest, PaletteThemeInputAndCycling)
{
    TuiRenderer renderer {};
    EXPECT_EQ(renderer.getPalette(), PaletteId::Classic);

    renderer.setPalette(PaletteId::Synthwave);
    EXPECT_EQ(renderer.getPalette(), PaletteId::Synthwave);

    renderer.cyclePalette();
    EXPECT_EQ(renderer.getPalette(), PaletteId::Amber);

    renderer.cyclePalette();
    EXPECT_EQ(renderer.getPalette(), PaletteId::Green);

    renderer.cyclePalette();
    EXPECT_EQ(renderer.getPalette(), PaletteId::Classic);

    TuiAction action {TuiAction::None};

    (void)renderer.processInput("p", action);
    EXPECT_EQ(action, TuiAction::CyclePalette);

    (void)renderer.processInput("P", action);
    EXPECT_EQ(action, TuiAction::CyclePalette);

    // F4 SS3 escape sequence: \033OS
    (void)renderer.processInput("\033OS", action);
    EXPECT_EQ(action, TuiAction::CyclePalette);

    // F4 VT220 escape sequence: \033[14~
    (void)renderer.processInput("\033[14~", action);
    EXPECT_EQ(action, TuiAction::CyclePalette);
}

TEST(TuiInputTest, ArtRevealInputAndToggle)
{
    TuiRenderer renderer {};
    EXPECT_TRUE(renderer.isArtEnabled());

    renderer.toggleArt();
    EXPECT_FALSE(renderer.isArtEnabled());
    renderer.toggleArt();
    EXPECT_TRUE(renderer.isArtEnabled());

    TuiAction action {TuiAction::None};

    (void)renderer.processInput("v", action);
    EXPECT_EQ(action, TuiAction::ToggleArt);

    (void)renderer.processInput("V", action);
    EXPECT_EQ(action, TuiAction::ToggleArt);

    // F5 escape sequence: \033[15~ -> QuickSave
    (void)renderer.processInput("\033[15~", action);
    EXPECT_EQ(action, TuiAction::QuickSave);

    // F9 escape sequence: \033[20~ -> QuickLoad
    (void)renderer.processInput("\033[20~", action);
    EXPECT_EQ(action, TuiAction::QuickLoad);
}
