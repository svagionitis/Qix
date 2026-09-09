#include "ReplaySystem.h"
#include "GameConfig.h"
#include "QixGame.h"
#include <cstdio>
#include <gtest/gtest.h>

TEST(ReplaySystemTest, CompactFormatRoundtrip)
{
    qix::ReplayHeader hdr {};
    hdr.version = 1;
    hdr.mode = qix::GameMode::Classic;
    hdr.playfieldWidth = 80;
    hdr.playfieldHeight = 60;
    hdr.targetPercent = 75;
    hdr.baseDelayMs = 50;
    hdr.seed = 1981;
    hdr.totalTicks = 200;
    hdr.finalScore = 15400;
    hdr.date = "2026-09-09";

    std::vector<qix::ReplayFrame> frames {{10, {qix::Direction::Right, qix::DrawMode::Slow}},
        {25, {qix::Direction::Down, qix::DrawMode::Slow}}, {40, {qix::Direction::Left, qix::DrawMode::Fast}}};

    const std::string text = qix::ReplaySystem::serializeCompact(hdr, frames);
    EXPECT_FALSE(text.empty());

    qix::ReplayHeader loadedHdr {};
    std::vector<qix::ReplayFrame> loadedFrames {};
    EXPECT_TRUE(qix::ReplaySystem::deserializeCompact(text, loadedHdr, loadedFrames));

    EXPECT_EQ(loadedHdr.version, hdr.version);
    EXPECT_EQ(loadedHdr.mode, hdr.mode);
    EXPECT_EQ(loadedHdr.playfieldWidth, hdr.playfieldWidth);
    EXPECT_EQ(loadedHdr.playfieldHeight, hdr.playfieldHeight);
    EXPECT_EQ(loadedHdr.targetPercent, hdr.targetPercent);
    EXPECT_EQ(loadedHdr.baseDelayMs, hdr.baseDelayMs);
    EXPECT_EQ(loadedHdr.totalTicks, hdr.totalTicks);
    EXPECT_EQ(loadedHdr.finalScore, hdr.finalScore);
    EXPECT_EQ(loadedHdr.date, hdr.date);

    ASSERT_EQ(loadedFrames.size(), 3U);
    EXPECT_EQ(loadedFrames[0].tick, 10U);
    EXPECT_EQ(loadedFrames[0].cmd.direction, qix::Direction::Right);
    EXPECT_EQ(loadedFrames[0].cmd.drawMode, qix::DrawMode::Slow);
    EXPECT_EQ(loadedFrames[1].tick, 25U);
    EXPECT_EQ(loadedFrames[1].cmd.direction, qix::Direction::Down);
    EXPECT_EQ(loadedFrames[1].cmd.drawMode, qix::DrawMode::Slow);
    EXPECT_EQ(loadedFrames[2].tick, 40U);
    EXPECT_EQ(loadedFrames[2].cmd.direction, qix::Direction::Left);
    EXPECT_EQ(loadedFrames[2].cmd.drawMode, qix::DrawMode::Fast);
}

TEST(ReplaySystemTest, JsonFormatRoundtrip)
{
    qix::ReplayHeader hdr {};
    hdr.version = 1;
    hdr.mode = qix::GameMode::Modern;
    hdr.playfieldWidth = 80;
    hdr.playfieldHeight = 60;
    hdr.targetPercent = 80;
    hdr.baseDelayMs = 60;
    hdr.seed = 2026;
    hdr.totalTicks = 350;
    hdr.finalScore = 24000;
    hdr.date = "2026-09-09T10:00:00Z";

    std::vector<qix::ReplayFrame> frames {
        {5, {qix::Direction::Up, qix::DrawMode::Fast}}, {18, {qix::Direction::Right, qix::DrawMode::Slow}}};

    const std::string json = qix::ReplaySystem::serializeJson(hdr, frames);
    EXPECT_FALSE(json.empty());

    qix::ReplayHeader loadedHdr {};
    std::vector<qix::ReplayFrame> loadedFrames {};
    EXPECT_TRUE(qix::ReplaySystem::deserializeJson(json, loadedHdr, loadedFrames));

    EXPECT_EQ(loadedHdr.mode, qix::GameMode::Modern);
    EXPECT_EQ(loadedHdr.playfieldWidth, 80);
    EXPECT_EQ(loadedHdr.targetPercent, 80U);
    EXPECT_EQ(loadedHdr.totalTicks, 350U);
    EXPECT_EQ(loadedHdr.finalScore, 24000U);

    ASSERT_EQ(loadedFrames.size(), 2U);
    EXPECT_EQ(loadedFrames[0].tick, 5U);
    EXPECT_EQ(loadedFrames[0].cmd.direction, qix::Direction::Up);
    EXPECT_EQ(loadedFrames[0].cmd.drawMode, qix::DrawMode::Fast);
    EXPECT_EQ(loadedFrames[1].tick, 18U);
    EXPECT_EQ(loadedFrames[1].cmd.direction, qix::Direction::Right);
    EXPECT_EQ(loadedFrames[1].cmd.drawMode, qix::DrawMode::Slow);
}

TEST(ReplaySystemTest, FileSaveAndLoadRoundtrip)
{
    const std::string testFile = "/tmp/test_roundtrip.qixrec";
    qix::ReplayHeader hdr {};
    hdr.mode = qix::GameMode::Classic;
    hdr.totalTicks = 100;
    hdr.finalScore = 5000;

    std::vector<qix::ReplayFrame> frames {{1, {qix::Direction::Left, qix::DrawMode::Slow}}};

    EXPECT_TRUE(qix::ReplaySystem::saveToFile(testFile, hdr, frames));

    qix::ReplayPlayer player {};
    EXPECT_TRUE(player.loadFromFile(testFile));
    EXPECT_TRUE(player.isLoaded());
    EXPECT_EQ(player.getHeader().finalScore, 5000U);
    EXPECT_EQ(player.getTotalTicks(), 100U);

    const auto cmd1 = player.getCommandForTick(1);
    EXPECT_EQ(cmd1.direction, qix::Direction::Left);
    EXPECT_EQ(cmd1.drawMode, qix::DrawMode::Slow);

    // Ticks without recorded commands return idle default
    const auto cmdIdle = player.getCommandForTick(2);
    EXPECT_EQ(cmdIdle.direction, qix::Direction::None);
    EXPECT_EQ(cmdIdle.drawMode, qix::DrawMode::None);

    std::remove(testFile.c_str());
}

TEST(ReplaySystemTest, DeterministicRegressionPlaythrough)
{
    // 1. Run live game simulation while recording
    qix::ReplayRecorder recorder {};
    qix::ReplayHeader recordHdr {};
    recordHdr.mode = qix::GameMode::Classic;
    recordHdr.playfieldWidth = 80;
    recordHdr.playfieldHeight = 60;
    recordHdr.targetPercent = 75;
    recordHdr.baseDelayMs = 50;

    recorder.start(recordHdr);

    qix::QixGame liveGame {80, 60, 75, qix::GameMode::Classic, 50};

    // Draw a small stix into field and return to border
    // Initial marker starts at (40, 59)
    for (std::uint32_t tick = 0; tick < 15; ++tick) {
        qix::PlayerCommand cmd {qix::Direction::Up, qix::DrawMode::Slow};
        recorder.recordTick(tick, cmd);
        liveGame.handleInput(cmd);
        liveGame.step(50);
    }
    for (std::uint32_t tick = 15; tick < 25; ++tick) {
        qix::PlayerCommand cmd {qix::Direction::Right, qix::DrawMode::Slow};
        recorder.recordTick(tick, cmd);
        liveGame.handleInput(cmd);
        liveGame.step(50);
    }
    for (std::uint32_t tick = 25; tick < 40; ++tick) {
        qix::PlayerCommand cmd {qix::Direction::Down, qix::DrawMode::Slow};
        recorder.recordTick(tick, cmd);
        liveGame.handleInput(cmd);
        liveGame.step(50);
    }
    for (std::uint32_t tick = 40; tick < 50; ++tick) {
        // Idle wait ticks
        qix::PlayerCommand cmd {qix::Direction::None, qix::DrawMode::None};
        recorder.recordTick(tick, cmd);
        liveGame.handleInput(cmd);
        liveGame.step(50);
    }

    const auto liveView = liveGame.getView();
    recorder.finish(liveView.stats.score, 50);
    EXPECT_FALSE(recorder.isRecording());

    const std::string serialized = qix::ReplaySystem::serializeCompact(recorder.getHeader(), recorder.getFrames());

    // 2. Play back recorded stream in a brand-new QixGame instance
    qix::ReplayPlayer player {};
    EXPECT_TRUE(player.loadFromCompact(serialized));

    qix::QixGame replayGame {80, 60, 75, qix::GameMode::Classic, 50};
    for (std::uint32_t tick = 0; tick < 50; ++tick) {
        const auto cmd = player.getCommandForTick(tick);
        replayGame.handleInput(cmd);
        replayGame.step(50);
    }

    // 3. Assert bit-identical regression equality
    const auto replayView = replayGame.getView();
    EXPECT_EQ(replayView.markerPos, liveView.markerPos);
    EXPECT_EQ(replayView.stats.score, liveView.stats.score);
    EXPECT_EQ(replayView.stats.claimedCells, liveView.stats.claimedCells);
    EXPECT_EQ(replayView.stats.claimedPercent, liveView.stats.claimedPercent);
    EXPECT_EQ(replayView.stats.lives, liveView.stats.lives);
}

TEST(ReplaySystemTest, CorruptOrEmptyFileHandling)
{
    qix::ReplayPlayer player {};
    EXPECT_FALSE(player.loadFromFile("/non/existent/path/qix.qixrec"));
    EXPECT_FALSE(player.loadFromJson(""));
    EXPECT_FALSE(player.loadFromJson("{ not valid json"));
    EXPECT_FALSE(player.isLoaded());
}

TEST(ReplaySystemTest, CliFlagsParsing)
{
    const std::vector<std::string> argsRecord {"./qix", "--record=gameplay.qixrec", "--mode=classic"};
    EXPECT_EQ(qix::GameConfig::parseRecordFlag(argsRecord), "gameplay.qixrec");
    EXPECT_TRUE(qix::GameConfig::parseReplayFlag(argsRecord).empty());

    const std::vector<std::string> argsShortRecord {"./qix", "-r", "session.json"};
    EXPECT_EQ(qix::GameConfig::parseRecordFlag(argsShortRecord), "session.json");

    const std::vector<std::string> argsReplay {"./qix", "--replay=gameplay.qixrec"};
    EXPECT_EQ(qix::GameConfig::parseReplayFlag(argsReplay), "gameplay.qixrec");
    EXPECT_TRUE(qix::GameConfig::parseRecordFlag(argsReplay).empty());

    const std::vector<std::string> argsPlayback {"./qix", "--playback=demo.qixrec"};
    EXPECT_EQ(qix::GameConfig::parseReplayFlag(argsPlayback), "demo.qixrec");
}
