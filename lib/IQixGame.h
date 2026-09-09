#pragma once
#include "Playfield.h"
#include "Types.h"
#include <deque>
#include <optional>
#include <string>
#include <vector>

namespace qix {

class HighScoreTable;

/// @brief Immutable view of the current game state rendered by clients.
struct GameView {
    const Playfield* playfield {nullptr};
    Point markerPos {0, 0};
    DrawMode drawMode {DrawMode::None};
    std::vector<Point> stixTrail {};
    std::vector<std::deque<LineSegment>> qixRibbons {};
    std::vector<Point> sparxPositions {};
    std::vector<SparxInfo> sparxList {};
    std::optional<Point> fusePos {std::nullopt};
    GameStats stats {};
    GameState state {GameState::Ready};
    GameMode mode {DefaultGameMode};
    NameEntryState nameEntry {};
    const HighScoreTable* highScoreTable {nullptr};
    bool isAttractMode {false};
    AttractStage attractStage {AttractStage::TitleScores};
    std::vector<GameEvent> events {};
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

    /// @brief Retrieve active game ruleset mode.
    /// @return Active GameMode (Classic or Modern).
    [[nodiscard]] virtual GameMode getGameMode() const noexcept = 0;

    /// @brief Retrieve baseline simulation tick delay in milliseconds.
    /// @return Baseline delay in milliseconds.
    [[nodiscard]] virtual std::uint32_t getBaseDelayMs() const noexcept = 0;

    /// @brief Retrieve active simulation tick delay for the current level in milliseconds.
    /// @return Active delay in milliseconds.
    [[nodiscard]] virtual std::uint32_t getCurrentDelayMs() const noexcept = 0;

    /// @brief Update baseline tick delay and recalculate active level delay.
    /// @param[in] delayMs New baseline tick delay in milliseconds.
    virtual void setBaseDelayMs(std::uint32_t delayMs) noexcept = 0;

    /// @brief Retrieve reference to Hall of Fame leaderboard.
    /// @return Reference to HighScoreTable.
    [[nodiscard]] virtual const HighScoreTable& getHighScoreTable() const noexcept = 0;

    /// @brief Process direct character input during 3-letter initials entry.
    /// @param[in] c Character entered by player.
    virtual void inputInitialsChar(char c) noexcept = 0;

    /// @brief Confirm and submit current initials in NameEntry state.
    virtual void confirmInitials() noexcept = 0;

    /// @brief Manually start the arcade attract showcase loop.
    virtual void startAttractMode() noexcept = 0;

    /// @brief Exit attract showcase and return to player Ready state.
    virtual void exitAttractMode() noexcept = 0;

    /// @brief Check whether game is currently running in attract showcase mode.
    /// @return True if in attract mode.
    [[nodiscard]] virtual bool isAttractMode() const noexcept = 0;

    /// @brief Save current game state to a save file (default: ~/.qix_saved_game.json).
    /// @param[in] filepath Destination file path (empty for default).
    /// @return True on success, false on failure.
    [[nodiscard]] virtual bool quickSave(const std::string& filepath = "") const noexcept = 0;

    /// @brief Load and resume game state from a save file (default: ~/.qix_saved_game.json).
    /// @param[in] filepath Source file path (empty for default).
    /// @return True on success, false on failure.
    [[nodiscard]] virtual bool quickLoad(const std::string& filepath = "") noexcept = 0;
};

} // namespace qix
