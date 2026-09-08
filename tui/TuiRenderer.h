#pragma once
#include "IQixGame.h"
#include <cstdint>
#include <string>

namespace qix::tui {

/// @brief Terminal screen resolution in character columns and rows.
struct TerminalSize {
    int cols {80};
    int rows {24};
};

/// @brief Actions triggered from terminal key inputs.
enum class TuiAction : std::uint8_t {
    None = 0,
    Quit,
    SpeedUp,
    SpeedDown,
    Restart,
    DisengageDraw,
    Confirm,
    ToggleBraille,
    Resize
};

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
    /// @param[out] action Special action (Quit, SpeedUp, SpeedDown, Restart, Confirm, ToggleBraille, Resize).
    /// @return PlayerCommand structure.
    [[nodiscard]] PlayerCommand pollInput(TuiAction& action) noexcept;

    /// @brief Enable or disable high-resolution Braille sub-pixel rendering.
    /// @param[in] enabled True for Braille sub-pixel rendering, false for ASCII grid.
    void setBrailleMode(bool enabled) noexcept;

    /// @brief Check whether Braille rendering mode is currently active.
    /// @return True if Braille mode is active.
    [[nodiscard]] bool isBrailleMode() const noexcept;

    /// @brief Toggle Braille rendering mode on/off.
    void toggleBrailleMode() noexcept;

    /// @brief Query the current terminal window dimensions.
    /// @return Terminal size in columns and rows.
    [[nodiscard]] static TerminalSize queryTerminalSize() noexcept;

    /// @brief Compute optimal playfield dimensions based on the terminal resolution.
    /// @param[in] brailleMode True if Braille sub-pixel mode is active.
    /// @return Pair containing width and height in playfield cell units.
    [[nodiscard]] static std::pair<std::int32_t, std::int32_t> computePlayfieldDimensions(
        bool brailleMode = true) noexcept;

    /// @brief Check if terminal size changed since last query and clear screen if so.
    /// @return True if terminal was resized.
    bool checkAndHandleResize() noexcept;

private:
    bool m_initialized {false};
    bool m_brailleMode {true};
    TerminalSize m_lastTermSize {0, 0};

    void clearScreen() noexcept;
    void renderAsciiPlayfield(std::string& frame, const GameView& view) noexcept;
    void renderBraillePlayfield(std::string& frame, const GameView& view) noexcept;
    void renderNameEntry(std::string& frame, const NameEntryState& entry, const GameStats& stats) noexcept;
    void renderHallOfFame(std::string& frame, const HighScoreTable* table, bool isGameOver) noexcept;
};

} // namespace qix::tui
