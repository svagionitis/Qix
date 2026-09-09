#include "GamePresenter.h"
#include "BackgroundArt.h"
#include "ColorPalette.h"
#include "PlayfieldViewport.h"
#include <gtest/gtest.h>

using namespace qix;

TEST(GamePresenterTest, InstructionRulesIntegrity)
{
    const auto& rules = GamePresenter::getInstructionRules();
    ASSERT_EQ(rules.size(), 5U);
    EXPECT_STREQ(rules[0].header, "OBJECTIVE:");
    EXPECT_STREQ(rules[1].header, "SLOW DRAW:");
    EXPECT_STREQ(rules[2].header, "FAST DRAW:");
    EXPECT_STREQ(rules[3].header, "HAZARDS:");
    EXPECT_STREQ(rules[4].header, "THE FUSE:");
}

TEST(GamePresenterTest, VictoryFormattingNormal)
{
    GameStats stats {};
    stats.claimedPercent = 78;
    stats.targetPercent = 75;
    stats.thresholdBonus = 3000;
    stats.splitBonus = false;
    stats.qixTrapped = false;

    const auto pres = GamePresenter::formatVictory(stats);
    EXPECT_EQ(pres.title, "LEVEL COMPLETE!");
    EXPECT_TRUE(pres.hasBonus);
    EXPECT_NE(pres.bonus.find("+3000 THRESHOLD BONUS (+3%)"), std::string::npos);
    EXPECT_FALSE(pres.isTrap);
    EXPECT_FALSE(pres.hasDetail);
}

TEST(GamePresenterTest, VictoryFormattingSpiralTrap)
{
    GameStats stats {};
    stats.qixTrapped = true;
    stats.spiralBonus = true;
    stats.trapBonus = 25000;
    stats.thresholdBonus = 5000;
    stats.qixRemainingPercent = 3;

    const auto pres = GamePresenter::formatVictory(stats);
    EXPECT_EQ(pres.title, "*** SPIRAL QIX TRAP! ***");
    EXPECT_TRUE(pres.isTrap);
    EXPECT_TRUE(pres.hasDetail);
    EXPECT_NE(pres.detail.find("CONFINED TO 3%"), std::string::npos);
    EXPECT_TRUE(pres.hasBonus);
    EXPECT_NE(pres.bonus.find("+25000 TRAP BONUS! (+5000 OVERSHOOT)"), std::string::npos);
}

TEST(GamePresenterTest, VictoryFormattingSplitBonus)
{
    GameStats stats {};
    stats.splitBonus = true;
    stats.multiplier = 3;

    const auto pres = GamePresenter::formatVictory(stats);
    EXPECT_EQ(pres.title, "QIX SPLIT BONUS!");
    EXPECT_NE(pres.prompt.find("Multiplier: 3X!"), std::string::npos);
}

TEST(GamePresenterTest, HudFormattingAndUrgency)
{
    GameStats stats {};
    stats.score = 12500;
    stats.highScore = 50000;
    stats.claimedPercent = 60;
    stats.targetPercent = 75;
    stats.timeRemainingMs = 25000; // 25s -> Normal
    stats.multiplier = 2;
    stats.level = 4;

    auto hud = GamePresenter::formatHud(stats, 16);
    EXPECT_EQ(hud.scoreStr, "12500");
    EXPECT_EQ(hud.hiScoreStr, "50000");
    EXPECT_EQ(hud.claimStr, "60% / 75%");
    EXPECT_FALSE(hud.targetReached);
    EXPECT_EQ(hud.timeUrgency, HudUrgency::Normal);
    EXPECT_EQ(hud.multiplierStr, "2X");
    EXPECT_EQ(hud.speedStr, "16ms");
    EXPECT_EQ(hud.levelStr, "4");

    // Warning tier (<= 20s)
    stats.timeRemainingMs = 15000;
    hud = GamePresenter::formatHud(stats, 16);
    EXPECT_EQ(hud.timeUrgency, HudUrgency::Warning);

    // Critical tier (<= 10s)
    stats.timeRemainingMs = 8000;
    hud = GamePresenter::formatHud(stats, 16);
    EXPECT_EQ(hud.timeUrgency, HudUrgency::Critical);
}

TEST(GamePresenterTest, HallOfFameRowsAndMedals)
{
    HighScoreTable table;
    (void)table.insert("AAA", 100000, 5, GameMode::Modern);
    (void)table.insert("BBB", 50000, 3, GameMode::Classic);
    (void)table.insert("CCC", 25000, 2, GameMode::Modern);
    (void)table.insert("DDD", 10000, 1, GameMode::Classic);

    const auto rows = GamePresenter::formatHallOfFame(&table, 4);
    ASSERT_EQ(rows.size(), 4U);
    EXPECT_EQ(rows[0].medal, MedalTier::Gold);
    EXPECT_EQ(rows[1].medal, MedalTier::Silver);
    EXPECT_EQ(rows[2].medal, MedalTier::Bronze);
    EXPECT_EQ(rows[3].medal, MedalTier::Standard);

    EXPECT_NE(rows[0].formattedRow.find("AAA"), std::string::npos);
    EXPECT_NE(rows[0].formattedRow.find("100000"), std::string::npos);
}

TEST(PlayfieldViewportTest, CoordinateTransforms)
{
    PlayfieldViewport vp {20.0f, 50.0f, 800.0f, 600.0f, 80, 60};
    EXPECT_FLOAT_EQ(vp.cellWidth(), 10.0f);
    EXPECT_FLOAT_EQ(vp.cellHeight(), 10.0f);

    const auto rect = vp.cellToScreen(5, 5, 0.5f);
    EXPECT_FLOAT_EQ(rect.x, 70.0f);
    EXPECT_FLOAT_EQ(rect.y, 100.0f);
    EXPECT_FLOAT_EQ(rect.width, 10.5f);
    EXPECT_FLOAT_EQ(rect.height, 10.5f);

    const auto pix = vp.cellToScreenPixel(5, 5);
    EXPECT_EQ(pix.x, 70);
    EXPECT_EQ(pix.y, 100);

    const auto src = vp.cellToTextureSrc(0, 0, 320, 240);
    EXPECT_EQ(src.x, 0);
    EXPECT_EQ(src.y, 0);
    EXPECT_EQ(src.width, 4);
    EXPECT_EQ(src.height, 4);

    EXPECT_EQ(PlayfieldViewport::centerX(200, 800), 300);
    EXPECT_TRUE(PlayfieldViewport::isBlinkOn(100, 350));
    EXPECT_FALSE(PlayfieldViewport::isBlinkOn(400, 350));
}

TEST(BackgroundArtTest, DualRgbaBuffersAndFastDrawTint)
{
    std::vector<std::uint8_t> stdBuf;
    std::vector<std::uint8_t> mutedBuf;
    BackgroundArt::generateDualRgbaBuffers(ArtScene::SynthwaveSunset, 32, 32, stdBuf, mutedBuf);

    ASSERT_EQ(stdBuf.size(), 32U * 32U * 4U);
    ASSERT_EQ(mutedBuf.size(), 32U * 32U * 4U);

    // Verify alpha is 255
    EXPECT_EQ(stdBuf[3], 255);
    EXPECT_EQ(mutedBuf[3], 255);

    PaletteColor raw {200, 100, 50, 255};
    PaletteColor tinted = BackgroundArt::tintForFastDraw(raw);
    EXPECT_EQ(tinted.r, raw.r / 3);
    EXPECT_EQ(tinted.a, 255);
}

TEST(ColorPaletteTest, ComputeRibbonColor)
{
    const auto& theme = ColorPalette::get(PaletteId::Synthwave);
    const auto c1 = ColorPalette::computeRibbonColor(theme, 50U, 0, 10);
    EXPECT_EQ(c1.a, 255);

    const auto& amber = ColorPalette::get(PaletteId::Amber);
    const auto c2 = ColorPalette::computeRibbonColor(amber, 0U, 9, 10);
    EXPECT_EQ(c2.a, 255);
    EXPECT_GT(c2.r, 0);
}
