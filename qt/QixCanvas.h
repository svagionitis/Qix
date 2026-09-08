#ifndef QIX_QT_CANVAS_H
#define QIX_QT_CANVAS_H

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

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    GameView m_view {};
    std::uint32_t m_colorCycle {0};
    std::uint32_t m_delayMs {75U};

    void drawHud(QPainter& painter);
    void drawPlayfield(QPainter& painter, const QRect& fieldRect);
    void drawQixRibbons(QPainter& painter, const QRect& fieldRect);
    void drawEntities(QPainter& painter, const QRect& fieldRect);
    void drawOverlays(QPainter& painter);
    void drawNameEntry(QPainter& painter);
    void drawHallOfFame(QPainter& painter, bool isGameOver);
};

} // namespace qix::qt

#endif // QIX_QT_CANVAS_H
