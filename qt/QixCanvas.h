#pragma once
#include "ColorPalette.h"
#include "IQixGame.h"
#include <QWidget>

namespace qix::qt {

/// @class QixCanvas
/// @brief Modern vector canvas rendering the Qix playfield, neon ribbons, and HUD.
class QixCanvas : public QWidget {
    Q_OBJECT

public:
    explicit QixCanvas(QWidget* parent = nullptr);
    ~QixCanvas() override = default;

    /// @brief Update canvas with the latest game snapshot and trigger repaint.
    /// @param[in] view Current game state snapshot.
    void updateView(const GameView& view);

    /// @brief Set the current simulation delay in milliseconds for HUD display.
    /// @param[in] delayMs Tick delay in milliseconds.
    void setDelayMs(std::uint32_t delayMs) noexcept;

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

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    GameView m_view {};
    std::uint32_t m_colorCycle {0};
    std::uint32_t m_delayMs {75U};
    bool m_crtEnabled {false};
    PaletteId m_paletteId {PaletteId::Classic};

    void applyCrtFilter(QPainter& painter, const QImage& sceneImage);

    void drawHud(QPainter& painter);
    void drawPlayfield(QPainter& painter, const QRect& fieldRect);
    void drawQixRibbons(QPainter& painter, const QRect& fieldRect);
    void drawEntities(QPainter& painter, const QRect& fieldRect);
    void drawOverlays(QPainter& painter);
    void drawNameEntry(QPainter& painter);
    void drawHallOfFame(QPainter& painter, bool isGameOver, bool isAttract = false);
    void drawDemoBanners(QPainter& painter);
    void drawInstructionsCard(QPainter& painter);
};

} // namespace qix::qt
