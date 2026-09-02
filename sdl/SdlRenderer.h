#ifndef QIX_SDL_RENDERER_H
#define QIX_SDL_RENDERER_H

#include "BitmapFont.h"
#include "IQixGame.h"
#include <SDL.h>
#include <cstdint>
#include <memory>
#include <string>

namespace qix::sdl {

/// @brief Custom deleter for SDL_Window.
struct SdlWindowDeleter {
    void operator()(SDL_Window* window) const noexcept
    {
        if (window != nullptr) {
            SDL_DestroyWindow(window);
        }
    }
};

/// @brief Custom deleter for SDL_Renderer.
struct SdlRendererDeleter {
    void operator()(SDL_Renderer* renderer) const noexcept
    {
        if (renderer != nullptr) {
            SDL_DestroyRenderer(renderer);
        }
    }
};

/// @class SdlRenderer
/// @brief Hardware-accelerated 2D renderer for Qix playfield, neon ribbons, and HUD using SDL2.
class SdlRenderer {
public:
    SdlRenderer() = default;
    ~SdlRenderer() = default;

    // Non-copyable
    SdlRenderer(const SdlRenderer&) = delete;
    SdlRenderer& operator=(const SdlRenderer&) = delete;

    // Movable
    SdlRenderer(SdlRenderer&&) noexcept = default;
    SdlRenderer& operator=(SdlRenderer&&) noexcept = default;

    /// @brief Initialize SDL video window and 2D hardware renderer.
    /// @param[in] title Window title text.
    /// @param[in] width Initial window width in pixels.
    /// @param[in] height Initial window height in pixels.
    /// @return True if initialization succeeded, false otherwise.
    [[nodiscard]] bool init(const std::string& title, int width, int height) noexcept;

    /// @brief Render complete game frame.
    /// @param[in] view Current game state snapshot.
    /// @param[in] delayMs Current tick delay in milliseconds.
    void render(const GameView& view, std::uint32_t delayMs) noexcept;

    /// @brief Present rendered back-buffer to the screen.
    void present() noexcept;

    /// @brief Get current window width in pixels.
    /// @return Current window width.
    [[nodiscard]] int getWidth() const noexcept;

    /// @brief Get current window height in pixels.
    /// @return Current window height.
    [[nodiscard]] int getHeight() const noexcept;

private:
    std::unique_ptr<SDL_Window, SdlWindowDeleter> m_window {nullptr};
    std::unique_ptr<SDL_Renderer, SdlRendererDeleter> m_renderer {nullptr};
    std::uint32_t m_colorCycle {0};

    void drawHud(const GameStats& stats, std::uint32_t delayMs) noexcept;
    void drawPlayfield(const Playfield& playfield, const SDL_Rect& fieldRect) noexcept;
    void drawQixRibbons(const std::vector<std::deque<LineSegment>>& ribbons, const SDL_Rect& fieldRect) noexcept;
    void drawEntities(const GameView& view, const SDL_Rect& fieldRect) noexcept;
    void drawOverlays(GameState state) noexcept;

    void drawFilledDiamond(int cx, int cy, int radius, SDL_Color color) noexcept;
    void drawThickLine(int x1, int y1, int x2, int y2, int thickness, SDL_Color color) noexcept;
    [[nodiscard]] static SDL_Color hsvToRgb(int hue, double sat, double val, std::uint8_t alpha) noexcept;
};

} // namespace qix::sdl

#endif // QIX_SDL_RENDERER_H
