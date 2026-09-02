#ifndef QIX_RAYLIB_RENDERER_H
#define QIX_RAYLIB_RENDERER_H

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

private:
    bool m_initialized {false};
    std::uint32_t m_colorCycle {0};

    void drawHud(const GameStats& stats, std::uint32_t delayMs) noexcept;
    void drawPlayfield(const Playfield& playfield, const Rectangle& fieldRect) noexcept;
    void drawQixRibbons(const std::vector<std::deque<LineSegment>>& ribbons, const Rectangle& fieldRect) noexcept;
    void drawEntities(const GameView& view, const Rectangle& fieldRect) noexcept;
    void drawOverlays(GameState state) noexcept;
};

} // namespace qix::raylib

#endif // QIX_RAYLIB_RENDERER_H
