#ifndef QIX_LIB_I_QIX_GAME_H
#define QIX_LIB_I_QIX_GAME_H

#include "Playfield.h"
#include "Types.h"
#include <deque>
#include <optional>
#include <vector>

namespace qix {

/// @brief Immutable view of the current game state rendered by clients.
struct GameView {
    const Playfield* playfield {nullptr};
    Point markerPos {0, 0};
    DrawMode drawMode {DrawMode::None};
    std::vector<Point> stixTrail {};
    std::vector<std::deque<LineSegment>> qixRibbons {};
    std::vector<Point> sparxPositions {};
    std::optional<Point> fusePos {std::nullopt};
    GameStats stats {};
    GameState state {GameState::Ready};
};

/// @class IQixGame
/// @brief Abstract interface defining the game engine API for renderers and controllers.
class IQixGame {
public:
    virtual ~IQixGame() = default;

    /// @brief Advance the simulation by one discrete tick.
    /// @param[in] deltaMs Milliseconds elapsed since previous step.
    virtual void step(std::uint32_t deltaMs) noexcept = 0;

    /// @brief Submit a player input command for the upcoming tick.
    /// @param[in] cmd Command structure containing direction and draw mode.
    virtual void handleInput(PlayerCommand cmd) noexcept = 0;

    /// @brief Retrieve immutable view snapshot of the current state.
    /// @return Reference to GameView.
    [[nodiscard]] virtual const GameView& getView() const noexcept = 0;

    /// @brief Reset game to initial state for a new session.
    virtual void reset() noexcept = 0;

    /// @brief Advance to next difficulty level.
    virtual void nextLevel() noexcept = 0;
};

} // namespace qix

#endif // QIX_LIB_I_QIX_GAME_H
