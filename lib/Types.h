#ifndef QIX_LIB_TYPES_H
#define QIX_LIB_TYPES_H

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

/// @brief Drawing mode determining speed and score multiplier.
enum class DrawMode : std::uint8_t { None = 0, Slow, Fast };

/// @brief State of the game session.
enum class GameState : std::uint8_t { Ready = 0, Playing, PlayerDying, LevelComplete, GameOver };

/// @brief Player input command.
struct PlayerCommand {
    Direction direction {Direction::None};
    DrawMode drawMode {DrawMode::None};
};

/// @brief Real-time game statistics.
struct GameStats {
    std::uint32_t score {0};
    std::uint32_t claimedCells {0};
    std::uint32_t totalEmptyCells {0};
    std::uint16_t claimedPercent {0};
    std::uint16_t targetPercent {75};
    std::uint8_t lives {3};
    std::uint8_t level {1};
};

/// @brief A line segment defined by two points.
struct LineSegment {
    Point start {0, 0};
    Point end {0, 0};
};

} // namespace qix

#endif // QIX_LIB_TYPES_H
