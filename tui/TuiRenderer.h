#pragma once
#include "ColorPalette.h"
#include "IQixGame.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

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
    ToggleTruecolor,
    CyclePalette,
    Resize,
    CharInput,
    Backspace
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
    /// @param[out] action Special action (Quit, SpeedUp, SpeedDown, Restart, Confirm, ToggleBraille, ToggleTruecolor,
    /// Resize, CharInput, Backspace).
    /// @return PlayerCommand structure.
    [[nodiscard]] PlayerCommand pollInput(TuiAction& action) noexcept;

    /// @brief Parse raw input bytes from terminal stream into player command and TUI action.
    /// @param[in] bytes Raw byte sequence to process.
    /// @param[out] action Special TUI action detected.
    /// @return PlayerCommand structure.
    [[nodiscard]] PlayerCommand processInput(std::string_view bytes, TuiAction& action) noexcept;

    /// @brief Retrieve the last typed printable character from user input.
    /// @details Provides access to the character code captured by the most recent pollInput() call.
    /// @return Character code entered by the user, or 0 if none.
    [[nodiscard]] char getTypedChar() const noexcept;

    /// @brief Enable or disable high-resolution Braille sub-pixel rendering.
    /// @param[in] enabled True for Braille sub-pixel rendering, false for ASCII grid.
    void setBrailleMode(bool enabled) noexcept;

    /// @brief Check whether Braille rendering mode is currently active.
    /// @return True if Braille mode is active.
    [[nodiscard]] bool isBrailleMode() const noexcept;

    /// @brief Toggle Braille rendering mode on/off.
    void toggleBrailleMode() noexcept;

    /// @brief Enable or disable 24-bit Truecolor (RGB) rendering.
    /// @param[in] enabled True for 24-bit Truecolor ANSI, false for standard 16-color ANSI.
    void setTruecolor(bool enabled) noexcept;

    /// @brief Check whether 24-bit Truecolor is currently enabled.
    /// @return True if Truecolor is enabled.
    [[nodiscard]] bool isTruecolor() const noexcept;

    /// @brief Toggle 24-bit Truecolor mode on/off.
    void toggleTruecolor() noexcept;

    /// @brief Set active visual color palette theme.
    /// @param[in] id Palette identifier.
    void setPalette(PaletteId id) noexcept;

    /// @brief Get active visual color palette theme identifier.
    /// @return Active PaletteId.
    [[nodiscard]] PaletteId getPalette() const noexcept;

    /// @brief Cycle to next color palette theme.
    void cyclePalette() noexcept;

    /// @brief Enable or disable flicker-free differential screen updates.
    /// @param[in] enabled True to emit only modified lines, false for full redraws.
    void setDifferentialUpdates(bool enabled) noexcept;

    /// @brief Check whether differential screen updates are currently enabled.
    /// @return True if differential updates are enabled.
    [[nodiscard]] bool isDifferentialUpdates() const noexcept;

    /// @brief Invalidate screen cache to force a full redraw on the next frame.
    void invalidateScreen() noexcept;

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
    bool m_truecolor {true};
    bool m_differentialUpdates {true};
    PaletteId m_paletteId {PaletteId::Classic};
    char m_typedChar {0};
    std::uint32_t m_colorCycle {0};
    TerminalSize m_lastTermSize {0, 0};
    std::vector<std::string> m_prevLines {};
    std::string m_inputQueue {};

    void clearScreen() noexcept;
    void presentFrame(const std::string& frame) noexcept;
    void renderAsciiPlayfield(std::string& frame, const GameView& view) noexcept;
    void renderBraillePlayfield(std::string& frame, const GameView& view) noexcept;
    void renderNameEntry(std::string& frame, const NameEntryState& entry, const GameStats& stats) noexcept;
    void renderHallOfFame(std::string& frame, const HighScoreTable* table, bool isGameOver) noexcept;
};

} // namespace qix::tui
