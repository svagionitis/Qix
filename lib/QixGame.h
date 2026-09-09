#pragma once

#include "CollisionDetector.h"
#include "DemoBot.h"
#include "Fuse.h"
#include "HighScoreTable.h"
#include "IQixGame.h"
#include "Marker.h"
#include "Playfield.h"
#include "Qix.h"
#include "Sparx.h"
#include "SpeedConfig.h"
#include "TerritoryFill.h"
#include <memory>
#include <vector>

namespace qix {

/// @class QixGame
/// @brief Concrete implementation of the Qix game engine.
/// @details Coordinates the simulation step, physics, collision detection, and score.
class QixGame final : public IQixGame {
public:
    /// @brief Construct game engine with field dimensions, target claim threshold, ruleset mode, and base speed.
    /// @param[in] width Playfield width in cells (default: 80).
    /// @param[in] height Playfield height in cells (default: 60).
    /// @param[in] targetPercent Victory percentage threshold (default: 75).
    /// @param[in] mode Game ruleset mode (default: DefaultGameMode).
    /// @param[in] baseDelayMs Base tick delay in milliseconds (default: SpeedConfig::DefaultDelayMs).
    explicit QixGame(std::int32_t width = 80, std::int32_t height = 60, std::uint16_t targetPercent = 75,
        GameMode mode = DefaultGameMode, std::uint32_t baseDelayMs = SpeedConfig::DefaultDelayMs) noexcept;

    ~QixGame() override = default;

    void step(std::uint32_t deltaMs) noexcept override;
    void handleInput(PlayerCommand cmd) noexcept override;
    [[nodiscard]] const GameView& getView() const noexcept override;
    void reset() noexcept override;
    void nextLevel() noexcept override;
    [[nodiscard]] GameMode getGameMode() const noexcept override;
    [[nodiscard]] std::uint32_t getBaseDelayMs() const noexcept override;
    [[nodiscard]] std::uint32_t getCurrentDelayMs() const noexcept override;
    void setBaseDelayMs(std::uint32_t delayMs) noexcept override;

    [[nodiscard]] const HighScoreTable& getHighScoreTable() const noexcept override;
    void inputInitialsChar(char c) noexcept override;
    void confirmInitials() noexcept override;

    void startAttractMode() noexcept override;
    void exitAttractMode() noexcept override;
    [[nodiscard]] bool isAttractMode() const noexcept override;
    void setDemoDurationMs(std::uint32_t durationMs) noexcept override;
    [[nodiscard]] std::uint32_t getDemoDurationMs() const noexcept override;

    [[nodiscard]] bool quickSave(const std::string& filepath = "") const noexcept override;
    [[nodiscard]] bool quickLoad(const std::string& filepath = "") noexcept override;

    /// @brief Calculate initial level time budget in milliseconds.
    /// @param[in] level Current game level (1-indexed).
    /// @return Initial countdown duration in milliseconds.
    [[nodiscard]] static constexpr std::uint32_t computeLevelTimeMs(std::uint8_t level) noexcept
    {
        const auto decrement = static_cast<std::uint32_t>(level > 0 ? (level - 1) : 0) * 5000U;
        return (decrement >= 30000U) ? 30000U : (60000U - decrement);
    }

    /// @brief Calculate fuse stationary idle tick limit for a given level.
    /// @param[in] level Current game level (1-indexed).
    /// @return Idle tick threshold before fuse ignition.
    [[nodiscard]] static constexpr std::uint32_t computeFuseLimit(std::uint8_t level) noexcept
    {
        const auto decrement = static_cast<std::uint32_t>(level > 0 ? (level - 1) : 0) * 2U;
        return (decrement >= 15U) ? 10U : (25U - decrement);
    }

private:
    Playfield m_playfield;
    Marker m_marker;
    std::vector<Qix> m_qixList {};
    std::vector<Sparx> m_sparxList {};
    Fuse m_fuse;
    TerritoryFill m_fill;

    GameMode m_mode {DefaultGameMode};
    GameStats m_stats {};
    GameState m_state {GameState::Ready};
    GameView m_view {};
    std::vector<GameEvent> m_events {};
    PlayerCommand m_pendingCmd {};
    static constexpr std::uint32_t ExtraLifeInterval {50000U};

    std::uint32_t m_timeRemainingMs {60000};
    std::uint32_t m_baseDelayMs {SpeedConfig::DefaultDelayMs};
    std::uint32_t m_currentDelayMs {SpeedConfig::DefaultDelayMs};
    std::uint32_t m_nextExtraLifeScore {ExtraLifeInterval};

    HighScoreTable m_highScoreTable {};
    NameEntryState m_nameEntry {};

    DemoBot m_demoBot {};
    AttractStage m_attractStage {AttractStage::TitleScores};
    std::uint32_t m_idleTimerMs {0};
    std::uint32_t m_attractStageTimerMs {0};
    static constexpr std::uint32_t IdleTimeoutMs {20000U};
    static constexpr std::uint32_t TitleStageDurationMs {6000U};
    static constexpr std::uint32_t InstructionsStageDurationMs {6000U};
    static constexpr std::uint32_t DefaultDemoStageDurationMs {90000U};
    std::uint32_t m_demoStageDurationMs {DefaultDemoStageDurationMs};

    void setupEntities() noexcept;
    void updateSnapshot() noexcept;
    void handleDeath() noexcept;
    void clearActiveStix() noexcept;
    void updateLevelTimer(std::uint32_t deltaMs) noexcept;
    void spawnEscalationSparx() noexcept;
    void handleNameEntryInput(PlayerCommand cmd) noexcept;
    void updateAttractCycle(std::uint32_t deltaMs) noexcept;
    void resetDemoPlayfield() noexcept;
};

} // namespace qix
