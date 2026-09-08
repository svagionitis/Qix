#include "ColorPalette.h"
#include <gtest/gtest.h>

TEST(ColorPaletteTest, ThemesHaveDistinctColors)
{
    const auto& classic = qix::ColorPalette::get(qix::PaletteId::Classic);
    const auto& synthwave = qix::ColorPalette::get(qix::PaletteId::Synthwave);
    const auto& amber = qix::ColorPalette::get(qix::PaletteId::Amber);
    const auto& green = qix::ColorPalette::get(qix::PaletteId::Green);

    EXPECT_EQ(classic.id, qix::PaletteId::Classic);
    EXPECT_EQ(synthwave.id, qix::PaletteId::Synthwave);
    EXPECT_EQ(amber.id, qix::PaletteId::Amber);
    EXPECT_EQ(green.id, qix::PaletteId::Green);

    // Ensure distinct border colors across themes
    EXPECT_NE(classic.playfieldBorder, synthwave.playfieldBorder);
    EXPECT_NE(classic.playfieldBorder, amber.playfieldBorder);
    EXPECT_NE(amber.playfieldBorder, green.playfieldBorder);

    // Ensure distinct background colors
    EXPECT_NE(classic.background, synthwave.background);
    EXPECT_NE(classic.background, amber.background);
    EXPECT_NE(classic.background, green.background);
}

TEST(ColorPaletteTest, NextCyclesThroughAllThemes)
{
    auto current = qix::PaletteId::Classic;
    current = qix::ColorPalette::next(current);
    EXPECT_EQ(current, qix::PaletteId::Synthwave);

    current = qix::ColorPalette::next(current);
    EXPECT_EQ(current, qix::PaletteId::Amber);

    current = qix::ColorPalette::next(current);
    EXPECT_EQ(current, qix::PaletteId::Green);

    current = qix::ColorPalette::next(current);
    EXPECT_EQ(current, qix::PaletteId::Classic);
}

TEST(ColorPaletteTest, FromNameParsesValidNames)
{
    EXPECT_EQ(qix::ColorPalette::fromName("classic"), qix::PaletteId::Classic);
    EXPECT_EQ(qix::ColorPalette::fromName("1981"), qix::PaletteId::Classic);
    EXPECT_EQ(qix::ColorPalette::fromName("DEFAULT"), qix::PaletteId::Classic);

    EXPECT_EQ(qix::ColorPalette::fromName("synthwave"), qix::PaletteId::Synthwave);
    EXPECT_EQ(qix::ColorPalette::fromName("cyberpunk"), qix::PaletteId::Synthwave);
    EXPECT_EQ(qix::ColorPalette::fromName("neon"), qix::PaletteId::Synthwave);

    EXPECT_EQ(qix::ColorPalette::fromName("amber"), qix::PaletteId::Amber);
    EXPECT_EQ(qix::ColorPalette::fromName("p3"), qix::PaletteId::Amber);
    EXPECT_EQ(qix::ColorPalette::fromName("orange"), qix::PaletteId::Amber);

    EXPECT_EQ(qix::ColorPalette::fromName("green"), qix::PaletteId::Green);
    EXPECT_EQ(qix::ColorPalette::fromName("p1"), qix::PaletteId::Green);
    EXPECT_EQ(qix::ColorPalette::fromName("matrix"), qix::PaletteId::Green);

    EXPECT_EQ(qix::ColorPalette::fromName("unknown_theme", qix::PaletteId::Amber), qix::PaletteId::Amber);
}

TEST(ColorPaletteTest, ToStringReturnsDescriptiveLabels)
{
    EXPECT_STREQ(qix::ColorPalette::toString(qix::PaletteId::Classic), "Classic 1981");
    EXPECT_STREQ(qix::ColorPalette::toString(qix::PaletteId::Synthwave), "Synthwave Neon");
    EXPECT_STREQ(qix::ColorPalette::toString(qix::PaletteId::Amber), "P3 Amber CRT");
    EXPECT_STREQ(qix::ColorPalette::toString(qix::PaletteId::Green), "P1 Green CRT");
}
