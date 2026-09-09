#include "SaveSystem.h"
#include "QixGame.h"
#include <filesystem>
#include <fstream>
#include <gtest/gtest.h>

namespace qix::test {

TEST(SaveSystemTest, ExpandPathTilde)
{
    const auto expanded = SaveSystem::expandPath("~/.qix_saved_game.json");
    EXPECT_NE(expanded.find(".qix_saved_game.json"), std::string::npos);
    EXPECT_EQ(expanded.find('~'), std::string::npos);

    const auto regular = SaveSystem::expandPath("/tmp/some_file.json");
    EXPECT_EQ(regular, "/tmp/some_file.json");
}

TEST(SaveSystemTest, CellRleRoundtrip)
{
    const int32_t width = 10;
    const int32_t height = 10;
    std::vector<CellState> original(static_cast<size_t>(width * height), CellState::Empty);

    for (int32_t x = 0; x < width; ++x) {
        original[static_cast<size_t>(x)] = CellState::Border;
        original[static_cast<size_t>((height - 1) * width + x)] = CellState::Border;
    }
    for (int32_t y = 0; y < height; ++y) {
        original[static_cast<size_t>(y * width)] = CellState::Border;
        original[static_cast<size_t>(y * width + (width - 1))] = CellState::Border;
    }
    original[static_cast<size_t>(2 * width + 2)] = CellState::ClaimedSlow;
    original[static_cast<size_t>(2 * width + 3)] = CellState::ClaimedFast;

    const auto encoded = SaveSystem::encodeCellsRle(original);
    EXPECT_FALSE(encoded.empty());

    std::vector<CellState> decoded {};
    EXPECT_TRUE(SaveSystem::decodeCellsRle(encoded, static_cast<size_t>(width * height), decoded));
    ASSERT_EQ(decoded.size(), original.size());
    for (size_t i = 0; i < original.size(); ++i) {
        EXPECT_EQ(decoded[i], original[i]);
    }
}

TEST(SaveSystemTest, SnapshotJsonRoundtrip)
{
    GameStateSnapshot snapshot {};
    snapshot.version = 1;
    snapshot.date = "2026-09-09T12:00:00Z";
    snapshot.mode = GameMode::Classic;
    snapshot.state = GameState::Playing;
    snapshot.baseDelayMs = 25;
    snapshot.currentDelayMs = 25;
    snapshot.timeRemainingMs = 45000;
    snapshot.nextExtraLifeScore = 50000;

    snapshot.stats.score = 12500;
    snapshot.stats.claimedPercent = 35;
    snapshot.stats.targetPercent = 75;
    snapshot.stats.level = 3;
    snapshot.stats.lives = 4;

    snapshot.playfield.width = 20;
    snapshot.playfield.height = 15;
    std::vector<CellState> cells(static_cast<size_t>(20 * 15), CellState::Empty);
    cells[0] = CellState::Border;
    cells[1] = CellState::ClaimedSlow;
    snapshot.playfield.cellsRle = SaveSystem::encodeCellsRle(cells);

    snapshot.marker.position = {5, 7};
    snapshot.marker.drawMode = DrawMode::Slow;
    snapshot.marker.lives = 4;
    snapshot.marker.trail = {{5, 6}, {5, 7}};

    QixSnapshot qix {};
    qix.p1 = {10, 10};
    qix.p2 = {12, 11};
    qix.vx1 = 1;
    qix.vy1 = -1;
    qix.vx2 = -1;
    qix.vy2 = 1;
    snapshot.qixes.push_back(qix);

    SparxSnapshot sparx1 {};
    sparx1.position = {0, 5};
    sparx1.clockwise = true;
    SparxSnapshot sparx2 {};
    sparx2.position = {19, 10};
    sparx2.clockwise = false;
    snapshot.sparxList.push_back(sparx1);
    snapshot.sparxList.push_back(sparx2);

    snapshot.fuse.idleCounter = 42;
    snapshot.fuse.trailIndex = 1;
    snapshot.fuse.isBurning = true;
    snapshot.fuse.position = {5, 6};

    const auto json = SaveSystem::serializeJson(snapshot);
    EXPECT_FALSE(json.empty());

    GameStateSnapshot loaded {};
    ASSERT_TRUE(SaveSystem::deserializeJson(json, loaded));

    EXPECT_EQ(loaded.version, snapshot.version);
    EXPECT_EQ(loaded.mode, snapshot.mode);
    EXPECT_EQ(loaded.state, snapshot.state);
    EXPECT_EQ(loaded.baseDelayMs, snapshot.baseDelayMs);
    EXPECT_EQ(loaded.currentDelayMs, snapshot.currentDelayMs);

    EXPECT_EQ(loaded.stats.score, snapshot.stats.score);
    EXPECT_EQ(loaded.stats.claimedPercent, snapshot.stats.claimedPercent);
    EXPECT_EQ(loaded.stats.targetPercent, snapshot.stats.targetPercent);
    EXPECT_EQ(loaded.stats.level, snapshot.stats.level);
    EXPECT_EQ(loaded.stats.lives, snapshot.stats.lives);

    EXPECT_EQ(loaded.playfield.width, snapshot.playfield.width);
    EXPECT_EQ(loaded.playfield.height, snapshot.playfield.height);
    EXPECT_EQ(loaded.playfield.cellsRle, snapshot.playfield.cellsRle);

    EXPECT_EQ(loaded.marker.position, snapshot.marker.position);
    EXPECT_EQ(loaded.marker.drawMode, snapshot.marker.drawMode);
    EXPECT_EQ(loaded.marker.lives, snapshot.marker.lives);
    ASSERT_EQ(loaded.marker.trail.size(), snapshot.marker.trail.size());
    EXPECT_EQ(loaded.marker.trail[0], snapshot.marker.trail[0]);
    EXPECT_EQ(loaded.marker.trail[1], snapshot.marker.trail[1]);

    ASSERT_EQ(loaded.qixes.size(), 1U);
    EXPECT_EQ(loaded.qixes[0].p1, snapshot.qixes[0].p1);
    EXPECT_EQ(loaded.qixes[0].p2, snapshot.qixes[0].p2);
    EXPECT_EQ(loaded.qixes[0].vx1, snapshot.qixes[0].vx1);
    EXPECT_EQ(loaded.qixes[0].vy1, snapshot.qixes[0].vy1);

    ASSERT_EQ(loaded.sparxList.size(), 2U);
    EXPECT_EQ(loaded.sparxList[0].position, snapshot.sparxList[0].position);
    EXPECT_EQ(loaded.sparxList[0].clockwise, snapshot.sparxList[0].clockwise);
    EXPECT_EQ(loaded.sparxList[1].position, snapshot.sparxList[1].position);
    EXPECT_EQ(loaded.sparxList[1].clockwise, snapshot.sparxList[1].clockwise);

    EXPECT_EQ(loaded.fuse.idleCounter, snapshot.fuse.idleCounter);
    EXPECT_EQ(loaded.fuse.trailIndex, snapshot.fuse.trailIndex);
}

TEST(SaveSystemTest, SaveAndLoadFile)
{
    const auto tempPath = (std::filesystem::temp_directory_path() / "qix_save_file_test.json").string();

    GameStateSnapshot snapshot {};
    snapshot.version = 1;
    snapshot.playfield.width = 10;
    snapshot.playfield.height = 10;
    std::vector<CellState> cells(100, CellState::Empty);
    snapshot.playfield.cellsRle = SaveSystem::encodeCellsRle(cells);
    snapshot.stats.score = 5000;
    snapshot.stats.level = 2;

    EXPECT_TRUE(SaveSystem::saveToFile(tempPath, snapshot));
    EXPECT_TRUE(std::filesystem::exists(tempPath));

    GameStateSnapshot loaded {};
    ASSERT_TRUE(SaveSystem::loadFromFile(tempPath, loaded));
    EXPECT_EQ(loaded.stats.score, 5000);
    EXPECT_EQ(loaded.stats.level, 2);
    EXPECT_EQ(loaded.playfield.width, 10);
    EXPECT_EQ(loaded.playfield.height, 10);

    std::filesystem::remove(tempPath);
}

TEST(SaveSystemTest, CorruptFileHandling)
{
    const auto nonExistent = (std::filesystem::temp_directory_path() / "qix_non_existent_12345.json").string();
    GameStateSnapshot snapshot {};
    EXPECT_FALSE(SaveSystem::loadFromFile(nonExistent, snapshot));

    const auto corruptPath = (std::filesystem::temp_directory_path() / "qix_corrupt_test.json").string();
    {
        std::ofstream out(corruptPath);
        out << "INVALID JSON NOT OBJECT {{{";
    }
    EXPECT_FALSE(SaveSystem::loadFromFile(corruptPath, snapshot));

    {
        std::ofstream out(corruptPath);
        out << "{\"version\": 999, \"playfield\": {\"width\": 10, \"height\": 10}}";
    }
    // Unsupported version
    EXPECT_FALSE(SaveSystem::loadFromFile(corruptPath, snapshot));

    std::filesystem::remove(corruptPath);
}

TEST(SaveSystemTest, QixGameQuickSaveAndQuickLoad)
{
    const auto tempSavePath = (std::filesystem::temp_directory_path() / "qix_game_save_test.json").string();

    QixGame game(80, 60, 75, GameMode::Classic, 30);
    // Move marker along border
    PlayerCommand cmd {};
    cmd.direction = Direction::Right;
    cmd.drawMode = DrawMode::None;
    game.handleInput(cmd);
    game.step(30);
    game.handleInput(cmd);
    game.step(30);

    const auto originalView = game.getView();

    // QuickSave
    EXPECT_TRUE(game.quickSave(tempSavePath));
    EXPECT_TRUE(std::filesystem::exists(tempSavePath));

    // Change game state: start drawing out into the field (Up from bottom border)
    PlayerCommand changeCmd {};
    changeCmd.direction = Direction::Up;
    changeCmd.drawMode = DrawMode::Slow;
    for (int i = 0; i < 5; ++i) {
        game.handleInput(changeCmd);
        game.step(30);
    }
    const auto mutatedView = game.getView();
    EXPECT_NE(originalView.markerPos, mutatedView.markerPos);

    // QuickLoad
    EXPECT_TRUE(game.quickLoad(tempSavePath));
    const auto restoredView = game.getView();

    EXPECT_EQ(restoredView.markerPos, originalView.markerPos);
    EXPECT_EQ(restoredView.drawMode, originalView.drawMode);
    EXPECT_EQ(restoredView.stats.score, originalView.stats.score);
    EXPECT_EQ(restoredView.stats.lives, originalView.stats.lives);
    EXPECT_EQ(restoredView.stats.level, originalView.stats.level);
    EXPECT_EQ(restoredView.state, originalView.state);

    std::filesystem::remove(tempSavePath);
}

} // namespace qix::test
