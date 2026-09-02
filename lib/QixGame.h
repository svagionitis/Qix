#ifndef QIX_LIB_QIX_GAME_H
#define QIX_LIB_QIX_GAME_H

#include "CollisionDetector.h"
#include "Fuse.h"
#include "IQixGame.h"
#include "Marker.h"
#include "Playfield.h"
#include "Qix.h"
#include "Sparx.h"
#include "TerritoryFill.h"
#include <memory>
#include <vector>

namespace qix {

/// @class QixGame
/// @brief Concrete implementation of the Qix game engine.
/// @details Coordinates the simulation step, physics, collision detection, and score.
class QixGame final : public IQixGame {
public:
    /// @brief Construct game engine with field dimensions and target claim threshold.
    /// @param[in] width Playfield width in cells (default: 80).
    /// @param[in] height Playfield height in cells (default: 60).
    /// @param[in] targetPercent Victory percentage threshold (default: 75).
    explicit QixGame(std::int32_t width = 80, std::int32_t height = 60, std::uint16_t targetPercent = 75) noexcept;

    ~QixGame() override = default;

    void step(std::uint32_t deltaMs) noexcept override;
    void handleInput(PlayerCommand cmd) noexcept override;
    [[nodiscard]] const GameView& getView() const noexcept override;
    void reset() noexcept override;
    void nextLevel() noexcept override;

private:
    Playfield m_playfield;
    Marker m_marker;
    std::vector<Qix> m_qixList {};
    std::vector<Sparx> m_sparxList {};
    Fuse m_fuse;
    TerritoryFill m_fill;

    GameStats m_stats {};
    GameState m_state {GameState::Ready};
    GameView m_view {};
    PlayerCommand m_pendingCmd {};

    void setupEntities() noexcept;
    void updateSnapshot() noexcept;
    void handleDeath() noexcept;
    void clearActiveStix() noexcept;
};

} // namespace qix

#endif // QIX_LIB_QIX_GAME_H
