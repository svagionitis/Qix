#pragma once
#include <array>
#include <cstdint>
#include <vector>

namespace qix {

/// @brief Represents a 2D integer point on the grid.
struct Point {
    std::int32_t x {0};
    std::int32_t y {0};

    [[nodiscard]] constexpr bool operator==(const Point& other) const noexcept
    {
        return x == other.x && y == other.y;
    }

    [[nodiscard]] constexpr bool operator!=(const Point& other) const noexcept
    {
        return !(*this == other);
    }
};

/// @brief Movement direction on the 2D grid.
enum class Direction : std::uint8_t { None = 0, Up, Down, Left, Right };

/// @brief Cell state representation on the discrete playfield.
enum class CellState : std::uint8_t { Empty = 0, Border, ClaimedSlow, ClaimedFast, ActiveStix };

/// @brief Ruleset configuration governing movement constraints.
enum class GameMode : std::uint8_t {
    Classic = 0, ///< Strict 1981 arcade rules: navigation restricted to perimeter borders only.
    Modern = 1 ///< Modern rules: navigation permitted on both borders and inside claimed shapes.
};

#if defined(QIX_DEFAULT_CLASSIC_MODE) && (QIX_DEFAULT_CLASSIC_MODE == 0)
inline constexpr GameMode DefaultGameMode {GameMode::Modern};
#else
inline constexpr GameMode DefaultGameMode {GameMode::Classic};
#endif

/// @brief Drawing mode determining speed and score multiplier.
enum class DrawMode : std::uint8_t { None = 0, Slow, Fast };

/// @brief State of the game session.
enum class GameState : std::uint8_t { Ready = 0, Playing, PlayerDying, LevelComplete, GameOver, NameEntry, HallOfFame, Attract };

/// @brief Attract cycle phase during non-interactive arcade showcase.
enum class AttractStage : std::uint8_t {
    TitleScores = 0, ///< High scores, title logo, and insert coin banner.
    Instructions = 1, ///< Arcade how-to-play rules and scoring values card.
    GameplayDemo = 2 ///< Autonomous AI agent live gameplay simulation.
};

/// @brief Player input command.
struct PlayerCommand {
    Direction direction {Direction::None};
    DrawMode drawMode {DrawMode::None};
};

/// @brief Real-time game statistics.
struct GameStats {
    std::uint32_t score {0};
    std::uint32_t highScore {0};
    std::uint32_t claimedCells {0};
    std::uint32_t totalEmptyCells {0};
    std::uint16_t claimedPercent {0};
    std::uint16_t targetPercent {75};
    std::uint8_t lives {3};
    std::uint8_t level {1};
    GameMode mode {DefaultGameMode};
    std::uint8_t multiplier {1};
    bool splitBonus {false};
    std::uint32_t timeRemainingMs {60000};
    std::uint32_t totalLevelTimeMs {60000};
    bool timeUp {false};
    std::uint32_t currentDelayMs {75U};
    std::uint32_t thresholdBonus {0};
    std::uint32_t nextExtraLifeScore {50000U};
    bool isAttractMode {false};
    AttractStage attractStage {AttractStage::TitleScores};
    std::uint32_t attractTimerMs {0};
    bool qixTrapped {false};
    bool spiralBonus {false};
    std::uint16_t qixRemainingPercent {0};
    std::uint32_t trapBonus {0};
};

/// @brief Active 3-letter initials entry state for Hall of Fame qualification.
struct NameEntryState {
    std::array<char, 3> initials {'A', 'A', 'A'};
    std::uint8_t cursorIndex {0};
    std::size_t rank {0};
};

/// @brief A line segment defined by two points.
struct LineSegment {
    Point start {0, 0};
    Point end {0, 0};
};

/// @brief Sparx render snapshot descriptor.
struct SparxInfo {
    Point position {0, 0};
    bool isSuper {false};
};

/// @brief Discrete game simulation events for visual feedback and particle effects.
enum class GameEventType : std::uint8_t {
    None = 0,
    MarkerDeath,
    TerritoryCapture
};

/// @brief Event descriptor dispatched upon notable game state occurrences.
struct GameEvent {
    GameEventType type {GameEventType::None};
    Point position {0, 0};
    std::vector<Point> capturePerimeter {};
    DrawMode drawMode {DrawMode::None};
};

} // namespace qix
