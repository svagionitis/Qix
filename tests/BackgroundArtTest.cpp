#include "BackgroundArt.h"
#include <gtest/gtest.h>

using namespace qix;

TEST(BackgroundArtTest, SceneCountAndNames)
{
    EXPECT_EQ(BackgroundArt::SceneCount, 4U);
    EXPECT_STREQ(BackgroundArt::getSceneName(ArtScene::CyberpunkSkyline), "Cyberpunk Skyline");
    EXPECT_STREQ(BackgroundArt::getSceneName(ArtScene::SynthwaveSunset), "Synthwave Sunset");
    EXPECT_STREQ(BackgroundArt::getSceneName(ArtScene::CosmicNebula), "Cosmic Nebula");
    EXPECT_STREQ(BackgroundArt::getSceneName(ArtScene::LaserMandala), "Laser Mandala");
}

TEST(BackgroundArtTest, LevelToSceneMapping)
{
    EXPECT_EQ(BackgroundArt::getSceneForLevel(1), ArtScene::CyberpunkSkyline);
    EXPECT_EQ(BackgroundArt::getSceneForLevel(2), ArtScene::SynthwaveSunset);
    EXPECT_EQ(BackgroundArt::getSceneForLevel(3), ArtScene::CosmicNebula);
    EXPECT_EQ(BackgroundArt::getSceneForLevel(4), ArtScene::LaserMandala);
    EXPECT_EQ(BackgroundArt::getSceneForLevel(5), ArtScene::CyberpunkSkyline); // Wraps around
    EXPECT_EQ(BackgroundArt::getSceneForLevel(8), ArtScene::LaserMandala);
    EXPECT_EQ(BackgroundArt::getSceneForLevel(0), ArtScene::CyberpunkSkyline); // Zero level fallback
}

TEST(BackgroundArtTest, FromIndexWrapping)
{
    EXPECT_EQ(BackgroundArt::fromIndex(0), ArtScene::CyberpunkSkyline);
    EXPECT_EQ(BackgroundArt::fromIndex(1), ArtScene::SynthwaveSunset);
    EXPECT_EQ(BackgroundArt::fromIndex(2), ArtScene::CosmicNebula);
    EXPECT_EQ(BackgroundArt::fromIndex(3), ArtScene::LaserMandala);
    EXPECT_EQ(BackgroundArt::fromIndex(4), ArtScene::CyberpunkSkyline);
    EXPECT_EQ(BackgroundArt::fromIndex(-1), ArtScene::LaserMandala);
}

TEST(BackgroundArtTest, SamplePixelAlphaAlwaysOpaque)
{
    for (std::size_t i {0}; i < BackgroundArt::SceneCount; ++i) {
        const auto scene = static_cast<ArtScene>(i);
        const auto c1 = BackgroundArt::samplePixel(scene, 0.0f, 0.0f);
        const auto c2 = BackgroundArt::samplePixel(scene, 0.5f, 0.5f);
        const auto c3 = BackgroundArt::samplePixel(scene, 1.0f, 1.0f);

        EXPECT_EQ(c1.a, 255);
        EXPECT_EQ(c2.a, 255);
        EXPECT_EQ(c3.a, 255);
    }
}

TEST(BackgroundArtTest, CoordinateClampingNoCrash)
{
    for (std::size_t i {0}; i < BackgroundArt::SceneCount; ++i) {
        const auto scene = static_cast<ArtScene>(i);
        // Out-of-bounds coordinates must clamp cleanly
        const auto neg = BackgroundArt::samplePixel(scene, -2.5f, -10.0f);
        const auto pos = BackgroundArt::samplePixel(scene, 5.0f, 12.0f);

        EXPECT_EQ(neg.a, 255);
        EXPECT_EQ(pos.a, 255);
    }
}

TEST(BackgroundArtTest, GenerateRgbaBufferDimensions)
{
    std::vector<std::uint8_t> buffer;
    BackgroundArt::generateRgbaBuffer(ArtScene::SynthwaveSunset, 64, 48, buffer);

    EXPECT_EQ(buffer.size(), 64U * 48U * 4U);

    // Verify non-zero pixels exist (not all black)
    bool hasNonZero = false;
    for (auto byte : buffer) {
        if (byte > 0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(BackgroundArtTest, GenerateZeroDimensionBuffer)
{
    std::vector<std::uint8_t> buffer;
    BackgroundArt::generateRgbaBuffer(ArtScene::CyberpunkSkyline, 0, 10, buffer);
    EXPECT_TRUE(buffer.empty());

    BackgroundArt::generateRgbaBuffer(ArtScene::CyberpunkSkyline, 10, -5, buffer);
    EXPECT_TRUE(buffer.empty());
}
