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
    /// @brief Construct marker with starting position, lives, and game ruleset mode.
    /// @param[in] startPos Starting coordinates on the playfield.
    /// @param[in] lives Initial life count.
    /// @param[in] mode Game ruleset mode (Classic or Modern).
    explicit Marker(Point startPos, std::uint8_t lives = 3, GameMode mode = DefaultGameMode) noexcept;

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

    /// @brief Maximum allowable life count.
    static constexpr std::uint8_t MaxLives {9};

    /// @brief Query remaining lives.
    [[nodiscard]] std::uint8_t getLives() const noexcept;

    /// @brief Increment remaining lives by one (capped at MaxLives).
    void incrementLives() noexcept;

    /// @brief Decrement remaining lives by one upon player death.
    void decrementLives() noexcept;

    /// @brief Check whether marker is still alive.
    /// @return True if lives > 0.
    [[nodiscard]] bool isAlive() const noexcept;

    /// @brief Retrieve the active game mode ruleset.
    /// @return Active GameMode.
    [[nodiscard]] GameMode getGameMode() const noexcept;

    /// @brief Update the active game mode ruleset.
    /// @param[in] mode Game mode ruleset to apply.
    void setGameMode(GameMode mode) noexcept;

    /// @brief Check whether a cell state is walkable when not drawing according to current mode.
    /// @param[in] state Playfield cell state to evaluate.
    /// @return True if navigable under current GameMode.
    [[nodiscard]] bool isNavigable(CellState state) const noexcept;

    /// @brief Legacy helper to check if cell is border or claimed territory.
    /// @param[in] state Playfield cell state to evaluate.
    /// @return True if Border, ClaimedSlow, or ClaimedFast.
    [[nodiscard]] static bool isBorderOrClaimed(CellState state) noexcept;

private:
    Point m_position {0, 0};
    DrawMode m_drawMode {DrawMode::None};
    std::uint8_t m_lives {3};
    GameMode m_mode {DefaultGameMode};
    std::vector<Point> m_trail {};

    [[nodiscard]] static Point calculateNext(Point current, Direction dir) noexcept;
};

} // namespace qix

#endif // QIX_LIB_MARKER_H
