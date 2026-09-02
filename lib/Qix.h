#ifndef QIX_LIB_QIX_H
#define QIX_LIB_QIX_H

#include "Playfield.h"
#include "Types.h"
#include <cstdint>
#include <deque>
#include <random>

namespace qix {

/// @class Qix
/// @brief Kinematic bouncing stick boss wandering inside unclaimed territory.
/// @details Maintains a history of line segments to produce the classic trailing ribbon effect.
class Qix {
public:
    /// @brief Construct Qix with initial line segment and segment trail capacity.
    /// @param[in] startLine Initial line segment.
    /// @param[in] historyLength Number of trailing line segments (default: 8).
    explicit Qix(LineSegment startLine, std::size_t historyLength = 8) noexcept;

    /// @brief Advance the Qix by one simulation step.
    /// @param[in] field Reference to playfield for collision and bounce detection.
    void update(const Playfield& field) noexcept;

    /// @brief Retrieve all active line segments forming the Qix ribbon.
    /// @return Const reference to queue of line segments.
    [[nodiscard]] const std::deque<LineSegment>& getSegments() const noexcept;

    /// @brief Retrieve the head (leading) line segment.
    /// @return Current head LineSegment.
    [[nodiscard]] LineSegment getHead() const noexcept;

    /// @brief Check if a point is in close proximity to any segment of the Qix.
    /// @param[in] pt Coordinate to test.
    /// @return True if collision detected, false otherwise.
    [[nodiscard]] bool intersects(Point pt) const noexcept;

    /// @brief Check if an active Stix trail intersects any segment of the Qix.
    /// @param[in] trail List of points in the active Stix.
    /// @return True if intersection detected.
    [[nodiscard]] bool intersectsTrail(const std::vector<Point>& trail) const noexcept;

private:
    std::size_t m_maxHistory {8};
    std::deque<LineSegment> m_segments {};

    Point m_p1 {0, 0};
    Point m_p2 {0, 0};
    std::int32_t m_vx1 {1};
    std::int32_t m_vy1 {1};
    std::int32_t m_vx2 {-1};
    std::int32_t m_vy2 {1};

    std::mt19937 m_rng {1981};

    void bounceEndpoint(Point& p, std::int32_t& vx, std::int32_t& vy, const Playfield& field) noexcept;
    static bool isPointOnSegment(Point p, Point a, Point b) noexcept;
};

} // namespace qix

#endif // QIX_LIB_QIX_H
