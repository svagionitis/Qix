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
    /// @brief Construct Sparx with starting position, initial movement direction, game mode, and super status.
    /// @param[in] startPos Coordinate on perimeter.
    /// @param[in] clockwise True for clockwise traversal, false for counter-clockwise.
    /// @param[in] mode Active game ruleset mode (Classic or Modern).
    /// @param[in] isSuper True if this Sparx can chase down active Stix trails.
    explicit Sparx(
        Point startPos, bool clockwise = true, GameMode mode = DefaultGameMode, bool isSuper = false) noexcept;

    /// @brief Advance Sparx position along the perimeter or active Stix by one cell step.
    /// @param[in] field Reference to playfield to detect perimeter connectivity.
    void update(const Playfield& field) noexcept;

    /// @brief Retrieve current coordinates.
    /// @return Current Point position.
    [[nodiscard]] Point getPosition() const noexcept;

    /// @brief Check collision with player marker.
    /// @param[in] markerPos Coordinates of the player marker.
    /// @return True if positions match.
    [[nodiscard]] bool checkCollision(Point markerPos) const noexcept;

    /// @brief Check whether this is a Super Sparx capable of traversing active Stix.
    /// @return True if Super Sparx.
    [[nodiscard]] bool isSuper() const noexcept;

    /// @brief Configure Super Sparx state.
    /// @param[in] isSuper True to enable Super Sparx behavior.
    void setSuper(bool isSuper) noexcept;

    /// @brief Retrieve active game mode ruleset.
    /// @return Active GameMode.
    [[nodiscard]] GameMode getGameMode() const noexcept;

    /// @brief Update active game mode ruleset.
    /// @param[in] mode Game mode ruleset to apply.
    void setGameMode(GameMode mode) noexcept;

    /// @brief Check whether a cell state is a valid traversable perimeter cell.
    /// @param[in] state Cell state to evaluate.
    /// @return True if traversable under current GameMode.
    [[nodiscard]] bool isPerimeter(CellState state) const noexcept;

    /// @brief Legacy helper to check if cell is border or claimed territory.
    /// @param[in] state Cell state to evaluate.
    /// @return True if Border, ClaimedSlow, or ClaimedFast.
    [[nodiscard]] static bool isPerimeterOrClaimed(CellState state) noexcept;

private:
    Point m_position {0, 0};
    bool m_clockwise {true};
    Direction m_lastDir {Direction::Right};
    GameMode m_mode {DefaultGameMode};
    bool m_isSuper {false};
};

} // namespace qix

#endif // QIX_LIB_SPARX_H
