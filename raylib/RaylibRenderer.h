#pragma once
#include "BackgroundArt.h"
#include "ColorPalette.h"
#include "IQixGame.h"
#include "ParticleSystem.h"
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

    /// @brief Set active visual color palette theme.
    /// @param[in] id Palette identifier.
    void setPalette(PaletteId id) noexcept;

    /// @brief Get active visual color palette theme identifier.
    /// @return Active PaletteId.
    [[nodiscard]] PaletteId getPalette() const noexcept;

    /// @brief Cycle to next color palette theme.
    void cyclePalette() noexcept;

    /// @brief Enable or disable background art reveal mode.
    /// @param[in] enabled True to reveal background artwork in claimed cells.
    void setArtEnabled(bool enabled) noexcept;

    /// @brief Check whether background art reveal mode is enabled.
    /// @return True if art reveal mode is active.
    [[nodiscard]] bool isArtEnabled() const noexcept;

    /// @brief Toggle background art reveal mode on/off at runtime.
    void toggleArt() noexcept;

    /// @brief Force a specific artwork scene, or -1 for auto level-based.
    /// @param[in] scene Scene index (0..3) or -1 for auto.
    void setArtScene(int scene) noexcept;

private:
    bool m_initialized {false};
    bool m_crtEnabled {false};
    bool m_artEnabled {true};
    int m_forcedArtScene {-1};
    PaletteId m_paletteId {PaletteId::Classic};
    RenderTexture2D m_targetTexture {};
    bool m_targetInitialized {false};
    Texture2D m_artTexture {};
    bool m_artTextureInitialized {false};
    ArtScene m_currentScene {ArtScene::Count};
    std::uint32_t m_colorCycle {0};

    void ensureArtTexture(ArtScene scene) noexcept;

    void drawHud(const GameStats& stats, std::uint32_t delayMs) noexcept;
    void drawPlayfield(const Playfield& playfield, const Rectangle& fieldRect, const GameView& view) noexcept;
    void drawQixRibbons(const std::vector<std::deque<LineSegment>>& ribbons, const Rectangle& fieldRect) noexcept;
    void drawEntities(const GameView& view, const Rectangle& fieldRect) noexcept;
    void drawParticles(const Rectangle& fieldRect) noexcept;
    void drawOverlays(const GameView& view, const Rectangle& fieldRect) noexcept;
    void drawNameEntry(const NameEntryState& entry, const GameStats& stats) noexcept;
    void drawHallOfFame(const HighScoreTable* table, bool isGameOver, bool isAttract = false) noexcept;
    void drawDemoBanners() noexcept;
    void drawInstructionsCard() noexcept;

    void applyCrtFilter(int width, int height) noexcept;

    ParticleSystem m_particles {};
};

} // namespace qix::raylib
