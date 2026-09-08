#include "ArcadeAudio.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <gtest/gtest.h>

using namespace qix;

TEST(ArcadeAudioTest, DefaultMutedProducesSilence)
{
    ArcadeAudio audio {};
    EXPECT_TRUE(audio.isMuted());

    std::array<std::int16_t, 512> buffer {};
    std::fill(buffer.begin(), buffer.end(), static_cast<std::int16_t>(1234));

    audio.generateSamples(buffer.data(), buffer.size());

    for (const auto sample : buffer) {
        EXPECT_EQ(sample, 0);
    }
}

TEST(ArcadeAudioTest, MuteToggleAndVolumeControl)
{
    ArcadeAudio audio {};
    EXPECT_TRUE(audio.isMuted());

    audio.toggleMute();
    EXPECT_FALSE(audio.isMuted());

    audio.setMuted(true);
    EXPECT_TRUE(audio.isMuted());

    audio.setMasterVolume(0.5f);
    EXPECT_FLOAT_EQ(audio.getMasterVolume(), 0.5f);

    audio.setMasterVolume(1.5f);
    EXPECT_FLOAT_EQ(audio.getMasterVolume(), 1.0f);

    audio.setMasterVolume(-0.2f);
    EXPECT_FLOAT_EQ(audio.getMasterVolume(), 0.0f);
}

TEST(ArcadeAudioTest, UnmutedActiveGameProducesAudioWaveform)
{
    ArcadeAudio audio {false}; // unmuted
    EXPECT_FALSE(audio.isMuted());

    GameView view {};
    view.state = GameState::Playing;

    std::deque<LineSegment> segments {};
    segments.push_back(LineSegment {Point {10, 10}, Point {30, 30}});
    view.qixRibbons.push_back(segments);

    audio.update(view, 50);

    std::array<std::int16_t, 1024> buffer {};
    audio.generateSamples(buffer.data(), buffer.size());

    bool hasNonZero = false;
    for (const auto sample : buffer) {
        if (sample != 0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(ArcadeAudioTest, DrawingChirpFastVsSlow)
{
    ArcadeAudio audioFast {false};
    ArcadeAudio audioSlow {false};

    GameView viewFast {};
    viewFast.state = GameState::Playing;
    viewFast.drawMode = DrawMode::Fast;

    GameView viewSlow {};
    viewSlow.state = GameState::Playing;
    viewSlow.drawMode = DrawMode::Slow;

    audioFast.update(viewFast, 50);
    audioSlow.update(viewSlow, 50);

    std::array<std::int16_t, 8820> bufferFast {};
    std::array<std::int16_t, 8820> bufferSlow {};

    audioFast.generateSamples(bufferFast.data(), bufferFast.size());
    audioSlow.generateSamples(bufferSlow.data(), bufferSlow.size());

    // Both should generate non-zero samples
    int nonZeroFast = 0;
    for (const auto s : bufferFast) {
        if (s != 0) {
            ++nonZeroFast;
        }
    }
    int nonZeroSlow = 0;
    for (const auto s : bufferSlow) {
        if (s != 0) {
            ++nonZeroSlow;
        }
    }

    EXPECT_GT(nonZeroFast, 0);
    EXPECT_GT(nonZeroSlow, 0);
    // Fast draw clicks at 18 Hz (more active click energy) compared to Slow draw at 9 Hz
    EXPECT_GT(nonZeroFast, nonZeroSlow);
}

TEST(ArcadeAudioTest, FuseSizzleActiveWhenFuseIgnited)
{
    ArcadeAudio audio {false};

    GameView view {};
    view.state = GameState::Playing;
    view.markerPos = Point {20, 20};
    view.fusePos = Point {15, 15}; // Fuse ignited nearby

    audio.update(view, 50);

    std::array<std::int16_t, 1024> buffer {};
    audio.generateSamples(buffer.data(), buffer.size());

    bool hasNonZero = false;
    for (const auto sample : buffer) {
        if (sample != 0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(ArcadeAudioTest, SparxSirenActiveWhenTimeUpOrClose)
{
    ArcadeAudio audio {false};

    GameView view {};
    view.state = GameState::Playing;
    view.markerPos = Point {10, 10};
    view.sparxPositions.push_back(Point {12, 10}); // very close Sparx (dist = 2)

    audio.update(view, 50);

    std::array<std::int16_t, 1024> buffer {};
    audio.generateSamples(buffer.data(), buffer.size());

    bool hasNonZero = false;
    for (const auto sample : buffer) {
        if (sample != 0) {
            hasNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(hasNonZero);
}

TEST(ArcadeAudioTest, LevelCompleteAndDeathTriggers)
{
    ArcadeAudio audio {false};

    audio.triggerLevelComplete();
    std::array<std::int16_t, 1024> fanfareBuffer {};
    audio.generateSamples(fanfareBuffer.data(), fanfareBuffer.size());

    bool fanfareNonZero = false;
    for (const auto sample : fanfareBuffer) {
        if (sample != 0) {
            fanfareNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(fanfareNonZero);

    audio.triggerPlayerDeath();
    std::array<std::int16_t, 1024> deathBuffer {};
    audio.generateSamples(deathBuffer.data(), deathBuffer.size());

    bool deathNonZero = false;
    for (const auto sample : deathBuffer) {
        if (sample != 0) {
            deathNonZero = true;
            break;
        }
    }
    EXPECT_TRUE(deathNonZero);
}
