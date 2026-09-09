#pragma once
#include "Playfield.h"
#include "SpeedConfig.h"
#include "Types.h"
#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace qix {

/// @struct QixSnapshot
/// @brief Serialized snapshot of a Qix entity including ribbon trail history and kinematics.
struct QixSnapshot {
    Point p1 {0, 0};
    Point p2 {0, 0};
    std::int32_t vx1 {1};
    std::int32_t vy1 {1};
    std::int32_t vx2 {-1};
    std::int32_t vy2 {1};
    std::deque<LineSegment> segments {};
};

/// @struct SparxSnapshot
/// @brief Serialized snapshot of a Sparx perimeter patroller.
struct SparxSnapshot {
    Point position {0, 0};
    bool clockwise {true};
    bool isSuper {false};
};

/// @struct FuseSnapshot
/// @brief Serialized snapshot of the Fuse anti-stall hazard.
struct FuseSnapshot {
    std::uint32_t idleLimit {30};
    std::uint32_t idleCounter {0};
    bool isBurning {false};
    std::size_t trailIndex {0};
    Point position {0, 0};
};

/// @struct MarkerSnapshot
/// @brief Serialized snapshot of player marker position, drawing state, lives, and active Stix trail.
struct MarkerSnapshot {
    Point position {0, 0};
    DrawMode drawMode {DrawMode::None};
    std::uint8_t lives {3};
    std::vector<Point> trail {};
};

/// @struct PlayfieldSnapshot
/// @brief Serialized representation of playfield grid dimensions and cell states.
struct PlayfieldSnapshot {
    std::int32_t width {80};
    std::int32_t height {60};
    std::string cellsRle {};
};

/// @struct GameStateSnapshot
/// @brief Complete state capture of an in-progress Qix game session.
struct GameStateSnapshot {
    std::uint32_t version {1};
    std::string date {};
    GameMode mode {DefaultGameMode};
    GameState state {GameState::Ready};
    std::uint32_t baseDelayMs {SpeedConfig::DefaultDelayMs};
    std::uint32_t currentDelayMs {SpeedConfig::DefaultDelayMs};
    std::uint32_t timeRemainingMs {60000};
    std::uint32_t nextExtraLifeScore {50000};

    GameStats stats {};
    PlayfieldSnapshot playfield {};
    MarkerSnapshot marker {};
    std::vector<QixSnapshot> qixes {};
    std::vector<SparxSnapshot> sparxList {};
    FuseSnapshot fuse {};
};

/// @class SaveSystem
/// @brief Serialization and persistence manager for saving and resuming game sessions.
class SaveSystem {
public:
    static constexpr std::uint32_t kSaveVersion {1};

    /// @brief Retrieve default save game file path (~/.qix_saved_game.json).
    /// @return Absolute or canonical path to default save file.
    [[nodiscard]] static std::string getDefaultFilePath() noexcept;

    /// @brief Resolve leading tilde '~' to the user's home directory.
    /// @param[in] path Path string potentially starting with '~'.
    /// @return Expanded filesystem path.
    [[nodiscard]] static std::string expandPath(const std::string& path) noexcept;

    /// @brief Save a game state snapshot to disk in JSON format.
    /// @param[in] filepath Target destination path.
    /// @param[in] snapshot State snapshot to serialize.
    /// @return True on success, false on I/O error.
    [[nodiscard]] static bool saveToFile(const std::string& filepath, const GameStateSnapshot& snapshot) noexcept;

    /// @brief Load and parse a game state snapshot from disk.
    /// @param[in] filepath Source JSON file path.
    /// @param[out] outSnapshot Destination snapshot structure.
    /// @return True on successful load and validation, false on error or corruption.
    [[nodiscard]] static bool loadFromFile(const std::string& filepath, GameStateSnapshot& outSnapshot) noexcept;

    /// @brief Serialize a game state snapshot into a formatted JSON string.
    /// @param[in] snapshot State snapshot.
    /// @return Serialized JSON string.
    [[nodiscard]] static std::string serializeJson(const GameStateSnapshot& snapshot) noexcept;

    /// @brief Parse a game state snapshot from a JSON string.
    /// @param[in] json JSON string.
    /// @param[out] outSnapshot Output snapshot.
    /// @return True if parsing succeeded and required fields exist, false otherwise.
    [[nodiscard]] static bool deserializeJson(const std::string& json, GameStateSnapshot& outSnapshot) noexcept;

    /// @brief Encode cell state vector into run-length encoded (RLE) string.
    /// @param[in] cells Vector of cell states.
    /// @return RLE string.
    [[nodiscard]] static std::string encodeCellsRle(const std::vector<CellState>& cells) noexcept;

    /// @brief Decode run-length encoded (RLE) string into cell state vector.
    /// @param[in] rle RLE string or raw character string.
    /// @param[in] expectedCount Expected total cell count (width * height).
    /// @param[out] outCells Output cell state vector.
    /// @return True on successful decode, false on syntax or length mismatch.
    [[nodiscard]] static bool decodeCellsRle(
        const std::string& rle, std::size_t expectedCount, std::vector<CellState>& outCells) noexcept;

    [[nodiscard]] static const char* gameStateToString(GameState state) noexcept;
    [[nodiscard]] static GameState stringToGameState(const std::string& str) noexcept;
    [[nodiscard]] static const char* cellStateToChar(CellState state) noexcept;
    [[nodiscard]] static CellState charToCellState(char ch) noexcept;
};

} // namespace qix
