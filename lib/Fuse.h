#pragma once
#include "Types.h"
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace qix {

/// @class Fuse
/// @brief Anti-stall hazard that ignites when a player stops moving while drawing a Stix.
/// @details Burns along the active Stix trail toward the player marker.
class Fuse {
public:
    /// @brief Construct a fuse with idle delay threshold before ignition.
    /// @param[in] idleTicksToIgnite Number of stationary ticks before ignition.
    explicit Fuse(std::uint32_t idleTicksToIgnite = 30) noexcept;

    /// @brief Update fuse state based on marker status.
    /// @param[in] isDrawing Whether marker is currently in drawing mode.
    /// @param[in] markerMoved Whether marker moved in the current tick.
    /// @param[in] trail Current Stix trail points.
    void update(bool isDrawing, bool markerMoved, const std::vector<Point>& trail) noexcept;

    /// @brief Extinguish and reset the fuse.
    void reset() noexcept;

    /// @brief Check if fuse is actively burning.
    /// @return True if burning.
    [[nodiscard]] bool isBurning() const noexcept;

    /// @brief Retrieve current burning coordinate on the trail.
    /// @return Optional Point containing position if burning.
    [[nodiscard]] std::optional<Point> getPosition() const noexcept;

    /// @brief Check if fuse has caught the player marker.
    /// @param[in] markerPos Coordinates of the marker.
    /// @return True if collision occurred.
    [[nodiscard]] bool checkCollision(Point markerPos) const noexcept;

    /// @brief Update stationary idle tick limit before ignition.
    /// @param[in] limit New idle tick threshold.
    void setIdleLimit(std::uint32_t limit) noexcept;

    /// @brief Retrieve current stationary idle tick limit.
    /// @return Current idle tick threshold.
    [[nodiscard]] std::uint32_t getIdleLimit() const noexcept;

    /// @brief Retrieve current stationary idle tick counter.
    /// @return Current idle tick count.
    [[nodiscard]] std::uint32_t getIdleCounter() const noexcept;

    /// @brief Retrieve current trail index reached by burning fuse.
    /// @return Index on active Stix trail.
    [[nodiscard]] std::size_t getTrailIndex() const noexcept;

    /// @brief Restore fuse state from saved snapshot.
    /// @param[in] idleLimit Saved idle limit.
    /// @param[in] idleCounter Saved idle counter.
    /// @param[in] burning Saved burning flag.
    /// @param[in] trailIndex Saved index on trail.
    /// @param[in] position Saved coordinates.
    void restore(std::uint32_t idleLimit, std::uint32_t idleCounter, bool burning, std::size_t trailIndex,
        Point position) noexcept;

private:
    std::uint32_t m_idleLimit {30};
    std::uint32_t m_idleCounter {0};
    bool m_burning {false};
    std::size_t m_trailIndex {0};
    Point m_position {0, 0};
};

} // namespace qix
