#pragma once
#include "BitmapFont.h"
#include "ColorPalette.h"
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

/// @brief Custom deleter for SDL_Texture.
struct SdlTextureDeleter {
    void operator()(SDL_Texture* texture) const noexcept
    {
        if (texture != nullptr) {
            SDL_DestroyTexture(texture);
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

    /// @brief Enable or disable CRT scanlines & phosphor glow post-processing filter.
    /// @param[in] enabled True to enable CRT filter, false for crisp modern display.
    void setCrtEnabled(bool enabled) noexcept;

    /// @brief Check if CRT scanlines & phosphor glow filter is currently active.
    /// @return True if CRT filter is enabled.
    [[nodiscard]] bool isCrtEnabled() const noexcept;

    /// @brief Toggle CRT scanlines & phosphor glow filter on/off.
    void toggleCrt() noexcept;

    /// @brief Set active visual color palette theme.
    /// @param[in] id Palette identifier.
    void setPalette(PaletteId id) noexcept;

    /// @brief Get active visual color palette theme identifier.
    /// @return Active PaletteId.
    [[nodiscard]] PaletteId getPalette() const noexcept;

    /// @brief Cycle to next color palette theme.
    void cyclePalette() noexcept;

private:
    std::unique_ptr<SDL_Window, SdlWindowDeleter> m_window {nullptr};
    std::unique_ptr<SDL_Renderer, SdlRendererDeleter> m_renderer {nullptr};
    std::unique_ptr<SDL_Texture, SdlTextureDeleter> m_sceneTexture {nullptr};
    int m_textureWidth {0};
    int m_textureHeight {0};
    bool m_crtEnabled {false};
    PaletteId m_paletteId {PaletteId::Classic};
    std::uint32_t m_colorCycle {0};

    void drawHud(const GameStats& stats, std::uint32_t delayMs) noexcept;
    void drawPlayfield(const Playfield& playfield, const SDL_Rect& fieldRect) noexcept;
    void drawQixRibbons(const std::vector<std::deque<LineSegment>>& ribbons, const SDL_Rect& fieldRect) noexcept;
    void drawEntities(const GameView& view, const SDL_Rect& fieldRect) noexcept;
    void drawOverlays(const GameView& view) noexcept;
    void drawNameEntry(const NameEntryState& entry, const GameStats& stats) noexcept;
    void drawHallOfFame(const HighScoreTable* table, bool isGameOver, bool isAttract = false) noexcept;
    void drawDemoBanners() noexcept;
    void drawInstructionsCard() noexcept;

    void applyCrtFilter(int width, int height) noexcept;
    void drawFilledDiamond(int cx, int cy, int radius, SDL_Color color) noexcept;
    void drawThickLine(int x1, int y1, int x2, int y2, int thickness, SDL_Color color) noexcept;
    [[nodiscard]] static SDL_Color hsvToRgb(int hue, double sat, double val, std::uint8_t alpha) noexcept;
};

} // namespace qix::sdl
