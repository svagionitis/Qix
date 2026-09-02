#ifndef QIX_LIB_SPARX_H
#define QIX_LIB_SPARX_H

#include "Playfield.h"
#include "Types.h"
#include <cstdint>

namespace qix {

/// @class Sparx
/// @brief Perimeter patroller enemy traversing boundary and claimed cells.
/// @details Follows the edge of captured territory; kills the player on boundary contact.
class Sparx {
public:
    /// @brief Construct Sparx with starting position and initial movement direction.
    /// @param[in] startPos Coordinate on perimeter.
    /// @param[in] clockwise True for clockwise traversal, false for counter-clockwise.
    explicit Sparx(Point startPos, bool clockwise = true) noexcept;

    /// @brief Advance Sparx position along the perimeter by one cell step.
    /// @param[in] field Reference to playfield to detect perimeter connectivity.
    void update(const Playfield& field) noexcept;

    /// @brief Retrieve current coordinates.
    /// @return Current Point position.
    [[nodiscard]] Point getPosition() const noexcept;

    /// @brief Check collision with player marker.
    /// @param[in] markerPos Coordinates of the player marker.
    /// @return True if positions match.
    [[nodiscard]] bool checkCollision(Point markerPos) const noexcept;

private:
    Point m_position {0, 0};
    bool m_clockwise {true};
    Direction m_lastDir {Direction::Right};

    [[nodiscard]] static bool isPerimeter(CellState state) noexcept;
};

} // namespace qix

#endif // QIX_LIB_SPARX_H
