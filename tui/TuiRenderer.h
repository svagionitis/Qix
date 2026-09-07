#ifndef QIX_TUI_RENDERER_H
#define QIX_TUI_RENDERER_H

#include "IQixGame.h"
#include <cstdint>
#include <string>

namespace qix::tui {

/// @brief Actions triggered from terminal key inputs.
enum class TuiAction : std::uint8_t { None = 0, Quit, SpeedUp, SpeedDown, Restart, DisengageDraw, Confirm };

/// @class TuiRenderer
/// @brief Cross-platform terminal renderer displaying the playfield, Qix ribbons, and HUD.
class TuiRenderer {
public:
    TuiRenderer() noexcept;
    ~TuiRenderer() noexcept;

    /// @brief Initialize terminal display (ANSI or curses).
    void init() noexcept;

    /// @brief Restore terminal settings before exit.
    void shutdown() noexcept;

    /// @brief Render a full game frame.
    /// @param[in] view Immutable game snapshot.
    /// @param[in] delayMs Current tick delay in milliseconds.
    void render(const GameView& view, std::uint32_t delayMs = 75) noexcept;

    /// @brief Poll for a player command non-blockingly.
    /// @param[out] action Special action (Quit, SpeedUp, SpeedDown, Restart, Confirm).
    /// @return PlayerCommand structure.
    [[nodiscard]] PlayerCommand pollInput(TuiAction& action) noexcept;

private:
    bool m_initialized {false};
    void clearScreen() noexcept;
    void renderNameEntry(std::string& frame, const NameEntryState& entry, const GameStats& stats) noexcept;
    void renderHallOfFame(std::string& frame, const HighScoreTable* table, bool isGameOver) noexcept;
};

} // namespace qix::tui

#endif // QIX_TUI_RENDERER_H
