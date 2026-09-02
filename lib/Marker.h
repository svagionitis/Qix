#ifndef QIX_LIB_MARKER_H
#define QIX_LIB_MARKER_H

#include "Playfield.h"
#include "Types.h"
#include <cstdint>
#include <vector>

namespace qix {

/// @class Marker
/// @brief Represents the player cursor navigating borders and drawing Stix trails.
/// @details Maintains position, lives, active draw mode, and coordinates of the current trail.
class Marker {
public:
    /// @brief Construct marker with starting position and lives.
    /// @param[in] startPos Starting coordinates on the playfield.
    /// @param[in] lives Initial life count.
    explicit Marker(Point startPos, std::uint8_t lives = 3) noexcept;

    /// @brief Reset position to safe starting point and clear trail.
    /// @param[in] resetPos Safe position on boundary.
    void resetPosition(Point resetPos) noexcept;

    /// @brief Move the marker in response to a player command.
    /// @param[in] field Reference to active playfield for boundary verification.
    /// @param[in] cmd Direction and requested drawing mode.
    /// @return True if movement occurred, false if blocked.
    bool move(Playfield& field, PlayerCommand cmd) noexcept;

    /// @brief Check if marker is actively drawing a Stix line.
    /// @return True if drawing, false if stationary or on safe border.
    [[nodiscard]] bool isDrawing() const noexcept;

    /// @brief Query active drawing mode.
    /// @return Active DrawMode (None, Slow, Fast).
    [[nodiscard]] DrawMode getDrawMode() const noexcept;

    /// @brief Retrieve current coordinates.
    /// @return Point representing current position.
    [[nodiscard]] Point getPosition() const noexcept;

    /// @brief Retrieve immutable reference to current Stix trail points.
    /// @return Vector of Points forming the active trail.
    [[nodiscard]] const std::vector<Point>& getTrail() const noexcept;

    /// @brief Clear the recorded Stix trail points.
    void clearTrail() noexcept;

    /// @brief Query remaining lives.
    /// @return Number of lives.
    [[nodiscard]] std::uint8_t getLives() const noexcept;

    /// @brief Decrement remaining lives by one upon player death.
    void decrementLives() noexcept;

    /// @brief Check whether marker is still alive.
    /// @return True if lives > 0.
    [[nodiscard]] bool isAlive() const noexcept;

private:
    Point m_position {0, 0};
    DrawMode m_drawMode {DrawMode::None};
    std::uint8_t m_lives {3};
    std::vector<Point> m_trail {};

    [[nodiscard]] static Point calculateNext(Point current, Direction dir) noexcept;
    [[nodiscard]] static bool isBorderOrClaimed(CellState state) noexcept;
};

} // namespace qix

#endif // QIX_LIB_MARKER_H
