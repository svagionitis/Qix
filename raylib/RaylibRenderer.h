#pragma once
#include "IQixGame.h"
#include <cstdint>
#include <deque>
#include <raylib.h>
#include <string>
#include <vector>

namespace qix::raylib {

/// @class RaylibRenderer
/// @brief Hardware-accelerated 2D renderer for Qix playfield, additive glowing ribbons, and HUD using Raylib.
class RaylibRenderer {
public:
    RaylibRenderer() = default;
    ~RaylibRenderer();

    // Non-copyable
    RaylibRenderer(const RaylibRenderer&) = delete;
    RaylibRenderer& operator=(const RaylibRenderer&) = delete;

    // Movable
    RaylibRenderer(RaylibRenderer&&) noexcept = default;
    RaylibRenderer& operator=(RaylibRenderer&&) noexcept = default;

    /// @brief Initialize Raylib display window and hardware-accelerated context.
    /// @param[in] title Window title text.
    /// @param[in] width Initial window width in pixels.
    /// @param[in] height Initial window height in pixels.
    /// @return True if window was successfully initialized, false otherwise.
    [[nodiscard]] bool init(const std::string& title, int width, int height) noexcept;

    /// @brief Render complete game frame.
    /// @param[in] view Current game state snapshot.
    /// @param[in] delayMs Current tick delay in milliseconds.
    void render(const GameView& view, std::uint32_t delayMs) noexcept;

    /// @brief Check if the window is currently initialized.
    /// @return True if initialized, false otherwise.
    [[nodiscard]] bool isInitialized() const noexcept;

    /// @brief Enable or disable CRT scanlines & phosphor glow filter.
    /// @param[in] enabled True to enable CRT filter.
    void setCrtEnabled(bool enabled) noexcept;

    /// @brief Check whether CRT filter is currently active.
    /// @return True if CRT filter is enabled.
    [[nodiscard]] bool isCrtEnabled() const noexcept;

    /// @brief Toggle CRT filter on/off at runtime.
    void toggleCrt() noexcept;

private:
    bool m_initialized {false};
    bool m_crtEnabled {false};
    RenderTexture2D m_targetTexture {};
    bool m_targetInitialized {false};
    std::uint32_t m_colorCycle {0};

    void drawHud(const GameStats& stats, std::uint32_t delayMs) noexcept;
    void drawPlayfield(const Playfield& playfield, const Rectangle& fieldRect) noexcept;
    void drawQixRibbons(const std::vector<std::deque<LineSegment>>& ribbons, const Rectangle& fieldRect) noexcept;
    void drawEntities(const GameView& view, const Rectangle& fieldRect) noexcept;
    void drawOverlays(const GameView& view) noexcept;
    void drawNameEntry(const NameEntryState& entry, const GameStats& stats) noexcept;
    void drawHallOfFame(const HighScoreTable* table, bool isGameOver) noexcept;

    void applyCrtFilter(int width, int height) noexcept;
};

} // namespace qix::raylib
