#include "QixCanvas.h"
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

namespace qix::gui {

QixCanvas::QixCanvas(QWidget* parent)
    : QWidget {parent}
{
    setMinimumSize(800, 640);
    setStyleSheet("background-color: #0b0f19;");
}

void QixCanvas::updateView(const GameView& view)
{
    m_view = view;
    ++m_colorCycle;
    update();
}

void QixCanvas::setDelayMs(std::uint32_t delayMs) noexcept
{
    m_delayMs = delayMs;
    update();
}

void QixCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Fill background
    painter.fillRect(rect(), QColor(11, 15, 25));

    drawHud(painter);

    // Compute playfield rect leaving top margin for HUD
    const int hudHeight = 50;
    const int margin = 20;
    QRect fieldRect(margin, hudHeight, width() - 2 * margin, height() - hudHeight - margin);

    if (m_view.playfield) {
        drawPlayfield(painter, fieldRect);
        drawQixRibbons(painter, fieldRect);
        drawEntities(painter, fieldRect);
    }

    drawOverlays(painter);
}

void QixCanvas::drawHud(QPainter& painter)
{
    painter.save();

    // Top status bar background
    painter.fillRect(0, 0, width(), 45, QColor(18, 24, 38));
    painter.setPen(QPen(QColor(35, 45, 68), 1));
    painter.drawLine(0, 45, width(), 45);

    QFont font("Monospace", 10, QFont::Bold);
    painter.setFont(font);

    // Score
    painter.setPen(QColor(160, 174, 192));
    painter.drawText(20, 28, "SCORE:");
    painter.setPen(QColor(246, 224, 94));
    painter.drawText(75, 28, QString::number(m_view.stats.score));

    // Claimed Percentage
    painter.setPen(QColor(160, 174, 192));
    painter.drawText(175, 28, "CLAIMED:");
    const auto percent = m_view.stats.claimedPercent;
    const auto target = m_view.stats.targetPercent;
    painter.setPen(percent >= target ? QColor(72, 187, 120) : QColor(99, 179, 237));
    painter.drawText(245, 28, QString("%1% / %2%").arg(percent).arg(target));

    // Progress Bar
    const int barX = 345;
    const int barY = 16;
    const int barW = 110;
    const int barH = 14;
    painter.setPen(Qt::NoPen);
    painter.fillRect(barX, barY, barW, barH, QColor(30, 41, 59));
    const int fillW = std::min(barW, (barW * percent) / 100);
    painter.fillRect(barX, barY, fillW, barH, percent >= target ? QColor(72, 187, 120) : QColor(59, 130, 246));

    // Lives
    painter.setPen(QColor(160, 174, 192));
    painter.drawText(475, 28, "LIVES:");
    for (int i = 0; i < m_view.stats.lives; ++i) {
        painter.setBrush(QColor(245, 101, 101));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(530 + i * 16, 18, 12, 12);
    }

    // Speed / Delay
    painter.setPen(QColor(160, 174, 192));
    painter.drawText(width() - 200, 28, "SPEED:");
    painter.setPen(QColor(56, 178, 172));
    painter.drawText(width() - 145, 28, QString("%1ms").arg(m_delayMs));

    // Level
    painter.setPen(QColor(160, 174, 192));
    painter.drawText(width() - 90, 28, "LEVEL:");
    painter.setPen(QColor(183, 148, 244));
    painter.drawText(width() - 35, 28, QString::number(m_view.stats.level));

    painter.restore();
}

void QixCanvas::drawPlayfield(QPainter& painter, const QRect& fieldRect)
{
    const auto gridW = m_view.playfield->getWidth();
    const auto gridH = m_view.playfield->getHeight();

    const double cellW = static_cast<double>(fieldRect.width()) / gridW;
    const double cellH = static_cast<double>(fieldRect.height()) / gridH;

    // Outer and interior cells
    for (std::int32_t y {0}; y < gridH; ++y) {
        for (std::int32_t x {0}; x < gridW; ++x) {
            const auto state = m_view.playfield->getCell(x, y);
            const QRectF r(fieldRect.left() + x * cellW, fieldRect.top() + y * cellH, cellW + 0.5, cellH + 0.5);

            if (state == CellState::Border) {
                painter.fillRect(r, QColor(59, 130, 246));
            } else if (state == CellState::ClaimedSlow) {
                painter.fillRect(r, QColor(14, 116, 144, 200));
            } else if (state == CellState::ClaimedFast) {
                painter.fillRect(r, QColor(180, 83, 9, 200));
            } else if (state == CellState::ActiveStix) {
                painter.fillRect(r, QColor(255, 255, 255));
            }
        }
    }
}

void QixCanvas::drawQixRibbons(QPainter& painter, const QRect& fieldRect)
{
    const auto gridW = m_view.playfield->getWidth();
    const auto gridH = m_view.playfield->getHeight();
    const double cellW = static_cast<double>(fieldRect.width()) / gridW;
    const double cellH = static_cast<double>(fieldRect.height()) / gridH;

    painter.save();

    for (const auto& ribbon : m_view.qixRibbons) {
        if (ribbon.empty()) {
            continue;
        }

        int segIndex = 0;
        const auto totalSegs = ribbon.size();

        for (const auto& seg : ribbon) {
            const double x1 = fieldRect.left() + (seg.start.x + 0.5) * cellW;
            const double y1 = fieldRect.top() + (seg.start.y + 0.5) * cellH;
            const double x2 = fieldRect.left() + (seg.end.x + 0.5) * cellW;
            const double y2 = fieldRect.top() + (seg.end.y + 0.5) * cellH;

            // Color cycle across segments to reproduce the iconic 1981 neon helix
            const int hue
                = static_cast<int>((m_colorCycle * 5 + segIndex * (360 / std::max<std::size_t>(1, totalSegs))) % 360);
            const int alpha = 255 - static_cast<int>((segIndex * 180) / std::max<std::size_t>(1, totalSegs));

            QColor lineColor = QColor::fromHsv(hue, 220, 255, alpha);
            painter.setPen(QPen(lineColor, (segIndex == 0) ? 3.0 : 2.0));
            painter.drawLine(QPointF(x1, y1), QPointF(x2, y2));

            ++segIndex;
        }
    }

    painter.restore();
}

void QixCanvas::drawEntities(QPainter& painter, const QRect& fieldRect)
{
    const auto gridW = m_view.playfield->getWidth();
    const auto gridH = m_view.playfield->getHeight();
    const double cellW = static_cast<double>(fieldRect.width()) / gridW;
    const double cellH = static_cast<double>(fieldRect.height()) / gridH;

    painter.save();

    // 1. Active Stix Trail
    if (m_view.stixTrail.size() >= 2) {
        QPainterPath trailPath;
        const auto start = m_view.stixTrail.front();
        trailPath.moveTo(fieldRect.left() + (start.x + 0.5) * cellW, fieldRect.top() + (start.y + 0.5) * cellH);
        for (std::size_t i = 1; i < m_view.stixTrail.size(); ++i) {
            const auto pt = m_view.stixTrail[i];
            trailPath.lineTo(fieldRect.left() + (pt.x + 0.5) * cellW, fieldRect.top() + (pt.y + 0.5) * cellH);
        }
        painter.setPen(QPen(QColor(255, 255, 255), 2.5));
        painter.drawPath(trailPath);
    }

    // 2. Sparx
    for (const auto& sp : m_view.sparxPositions) {
        const double cx = fieldRect.left() + (sp.x + 0.5) * cellW;
        const double cy = fieldRect.top() + (sp.y + 0.5) * cellH;

        // Glowing red/magenta diamond
        painter.setBrush(QColor(236, 72, 153));
        painter.setPen(QPen(QColor(255, 255, 255), 1.0));

        QPolygonF diamond;
        diamond << QPointF(cx, cy - 6) << QPointF(cx + 6, cy) << QPointF(cx, cy + 6) << QPointF(cx - 6, cy);
        painter.drawPolygon(diamond);
    }

    // 3. Fuse
    if (m_view.fusePos.has_value()) {
        const auto fp = m_view.fusePos.value();
        const double cx = fieldRect.left() + (fp.x + 0.5) * cellW;
        const double cy = fieldRect.top() + (fp.y + 0.5) * cellH;

        painter.setBrush(QColor(239, 68, 68));
        painter.setPen(QPen(QColor(254, 240, 138), 1.5));
        painter.drawEllipse(QPointF(cx, cy), 5, 5);
    }

    // 4. Player Marker
    const double mx = fieldRect.left() + (m_view.markerPos.x + 0.5) * cellW;
    const double my = fieldRect.top() + (m_view.markerPos.y + 0.5) * cellH;

    painter.setBrush(m_view.drawMode != DrawMode::None ? QColor(250, 204, 21) : QColor(243, 244, 246));
    painter.setPen(QPen(QColor(0, 0, 0), 1.0));

    QPolygonF markerDiamond;
    markerDiamond << QPointF(mx, my - 7) << QPointF(mx + 7, my) << QPointF(mx, my + 7) << QPointF(mx - 7, my);
    painter.drawPolygon(markerDiamond);

    painter.restore();
}

void QixCanvas::drawOverlays(QPainter& painter)
{
    if (m_view.state == GameState::Playing || m_view.state == GameState::Ready) {
        return;
    }

    painter.save();
    painter.fillRect(rect(), QColor(0, 0, 0, 170));

    QFont font("Monospace", 24, QFont::Bold);
    painter.setFont(font);

    if (m_view.state == GameState::LevelComplete) {
        painter.setPen(QColor(74, 222, 128));
        painter.drawText(rect(), Qt::AlignCenter, "LEVEL COMPLETE!\nPress [Space] for Next Level");
    } else if (m_view.state == GameState::GameOver) {
        painter.setPen(QColor(248, 113, 113));
        painter.drawText(rect(), Qt::AlignCenter, "GAME OVER\nPress [R] to Restart");
    }

    painter.restore();
}

} // namespace qix::gui
