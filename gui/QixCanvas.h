#ifndef QIX_GUI_CANVAS_H
#define QIX_GUI_CANVAS_H

#include "IQixGame.h"
#include <QWidget>

namespace qix::gui {

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

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    GameView m_view {};
    std::uint32_t m_colorCycle {0};

    void drawHud(QPainter& painter);
    void drawPlayfield(QPainter& painter, const QRect& fieldRect);
    void drawQixRibbons(QPainter& painter, const QRect& fieldRect);
    void drawEntities(QPainter& painter, const QRect& fieldRect);
    void drawOverlays(QPainter& painter);
};

} // namespace qix::gui

#endif // QIX_GUI_CANVAS_H
