#pragma once

#include "IQixGame.h"
#include "Types.h"
#include <cstdint>

namespace qix {

/// @class DemoBot
/// @brief Autonomous AI player controller for simulated gameplay during arcade Attract Mode.
/// @details Evaluates the real-time GameView and generates deterministic PlayerCommands to
/// showcase perimeter navigation, strategic Stix cuts, territory captures, and enemy avoidance.
class DemoBot {
public:
    /// @brief Construct a new DemoBot controller.
    DemoBot() noexcept = default;

    ~DemoBot() = default;

    // Default copyable and movable
    DemoBot(const DemoBot&) noexcept = default;
    DemoBot& operator=(const DemoBot&) noexcept = default;
    DemoBot(DemoBot&&) noexcept = default;
    DemoBot& operator=(DemoBot&&) noexcept = default;

    /// @brief Reset internal navigation state machine for a fresh demonstration run.
    void reset() noexcept;

    /// @brief Generate the next player command for the current simulation tick.
    /// @param[in] view Immutable game state snapshot.
    /// @return PlayerCommand containing chosen movement direction and drawing mode.
    [[nodiscard]] PlayerCommand update(const GameView& view) noexcept;

    /// @brief Raycast distance to nearest boundary or claimed territory in the given direction.
    /// @param[in] playfield Pointer to current Playfield.
    /// @param[in] p Starting point.
    /// @param[in] dir Direction of travel.
    /// @param[in] mode Classic or Modern ruleset.
    /// @return Number of steps to boundary, or 9999 if unbounded / out of bounds.
    [[nodiscard]] static std::int32_t getDistanceToBoundary(
        const Playfield* playfield, Point p, Direction dir, GameMode mode) noexcept;

    /// @brief Find the quickest safe direction to reach a boundary without trail self-intersection.
    /// @param[in] view Game view snapshot.
    /// @param[in] marker Current marker position.
    /// @param[in] trail Active Stix trail points.
    /// @return Best escape direction, or Direction::None if none valid.
    [[nodiscard]] Direction findQuickestSafeExit(
        const GameView& view, Point marker, const std::vector<Point>& trail) const noexcept;

    /// @brief Compute Euclidean distance to nearest Qix entity head.
    /// @param[in] view Game view snapshot.
    /// @param[in] p Observation point.
    /// @return Distance in grid units.
    [[nodiscard]] float getQixDistance(const GameView& view, Point p) const noexcept;

private:
    enum class State : std::uint8_t {
        BorderPatrol = 0,
        CuttingStraight = 1,
        CuttingInward = 2,
        CuttingParallel = 3,
        CuttingReturn = 4,
        EmergencyEscape = 5
    };

    State m_state {State::BorderPatrol};
    Direction m_borderDir {Direction::Right};
    Direction m_inwardDir {Direction::Up};
    Direction m_parallelDir {Direction::Right};
    Direction m_returnDir {Direction::Down};
    Direction m_escapeDir {Direction::None};

    std::uint32_t m_stepCount {0};
    std::uint32_t m_targetSteps {0};
    std::uint32_t m_patrolTicks {0};
    std::uint32_t m_stuckTicks {0};
    Point m_lastMarkerPos {0, 0};
    bool m_useSlowDraw {false};

    [[nodiscard]] bool isCellEmpty(const Playfield* playfield, Point p) const noexcept;
    [[nodiscard]] bool isCellBorder(const Playfield* playfield, Point p, GameMode mode) const noexcept;
    [[nodiscard]] bool isQixThreatening(const GameView& view, Point p, float dangerRadius) const noexcept;
    [[nodiscard]] bool isSparxNear(const GameView& view, Point p, float radius) const noexcept;
    [[nodiscard]] static Point stepPoint(Point p, Direction dir) noexcept;
    [[nodiscard]] static Direction oppositeDir(Direction dir) noexcept;
};

} // namespace qix
