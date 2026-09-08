#include "QixCanvas.h"
#include "HighScoreTable.h"
#include <QColor>
#include <QFont>
#include <QPainter>
#include <QPainterPath>
#include <cmath>

namespace {
[[nodiscard]] inline QColor toQColor(const qix::PaletteColor& c) noexcept
{
    return QColor(c.r, c.g, c.b, c.a);
}
} // namespace

namespace qix::qt {

QixCanvas::QixCanvas(QWidget* parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent, true);
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

void QixCanvas::setCrtEnabled(bool enabled) noexcept
{
    m_crtEnabled = enabled;
    update();
}

bool QixCanvas::isCrtEnabled() const noexcept
{
    return m_crtEnabled;
}

void QixCanvas::toggleCrt() noexcept
{
    m_crtEnabled = !m_crtEnabled;
    update();
}

void QixCanvas::setPalette(PaletteId id) noexcept
{
    m_paletteId = id;
    update();
}

PaletteId QixCanvas::getPalette() const noexcept
{
    return m_paletteId;
}

void QixCanvas::cyclePalette() noexcept
{
    m_paletteId = ColorPalette::next(m_paletteId);
    update();
}

void QixCanvas::setArtEnabled(bool enabled) noexcept
{
    m_artEnabled = enabled;
    update();
}

bool QixCanvas::isArtEnabled() const noexcept
{
    return m_artEnabled;
}

void QixCanvas::toggleArt() noexcept
{
    m_artEnabled = !m_artEnabled;
    update();
}

void QixCanvas::setArtScene(int scene) noexcept
{
    m_customArtScene = scene;
    m_artLevel = -1;
    update();
}

void QixCanvas::ensureArtImage()
{
    const auto scene = (m_customArtScene >= 0)
        ? BackgroundArt::fromIndex(m_customArtScene)
        : BackgroundArt::getSceneForLevel(m_view.stats.level);

    if (m_artLevel == m_view.stats.level && m_currentArtScene == scene && !m_artImage.isNull()) {
        return;
    }

    m_currentArtScene = scene;
    m_artLevel = m_view.stats.level;

    constexpr int ArtW = 640;
    constexpr int ArtH = 480;
    std::vector<std::uint8_t> rgba;
    BackgroundArt::generateRgbaBuffer(scene, ArtW, ArtH, rgba);

    QImage raw(rgba.data(), ArtW, ArtH, ArtW * 4, QImage::Format_RGBA8888);
    m_artImage = raw.copy();

    m_artMutedImage = m_artImage.copy();
    for (int y = 0; y < ArtH; ++y) {
        auto* scan = reinterpret_cast<QRgb*>(m_artMutedImage.scanLine(y));
        for (int x = 0; x < ArtW; ++x) {
            const QRgb p = scan[x];
            const int r = qRed(p) / 3;
            const int g = std::min(255, qGreen(p) * 2 / 3 + 30);
            const int b = std::min(255, qBlue(p) * 4 / 5 + 60);
            scan[x] = qRgba(r, g, b, 255);
        }
    }
}

void QixCanvas::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const int hudHeight = 50;
    const int margin = 20;
    QRect fieldRect(margin, hudHeight, width() - 2 * margin, height() - hudHeight - margin);
    const auto& theme = ColorPalette::get(m_paletteId);

    if (m_crtEnabled) {
        QImage sceneImage(size(), QImage::Format_ARGB32_Premultiplied);
        sceneImage.fill(toQColor(theme.background));

        QPainter imgPainter(&sceneImage);
        imgPainter.setRenderHint(QPainter::Antialiasing, true);

        drawHud(imgPainter);
        if (m_view.playfield) {
            drawPlayfield(imgPainter, fieldRect);
            drawQixRibbons(imgPainter, fieldRect);
            drawEntities(imgPainter, fieldRect);
        }
        drawOverlays(imgPainter);
        imgPainter.end();

        applyCrtFilter(painter, sceneImage);
    } else {
        // Fill background
        painter.fillRect(rect(), toQColor(theme.background));

        drawHud(painter);

        if (m_view.playfield) {
            drawPlayfield(painter, fieldRect);
            drawQixRibbons(painter, fieldRect);
            drawEntities(painter, fieldRect);
        }

        drawOverlays(painter);
    }
}

void QixCanvas::applyCrtFilter(QPainter& painter, const QImage& sceneImage)
{
    const auto& theme = ColorPalette::get(m_paletteId);
    // Clear background
    painter.fillRect(rect(), toQColor(theme.crtBackdrop));

    // 1. Draw base game scene
    painter.drawImage(0, 0, sceneImage);

    // 2. Additive phosphor glow bloom pass
    painter.save();
    painter.setCompositionMode(QPainter::CompositionMode_Plus);
    painter.setOpacity(0.35);

    const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& off : offsets) {
        painter.drawImage(off[0], off[1], sceneImage);
    }
    painter.setOpacity(0.18);
    const int wideOffsets[4][2] = {{-2, -1}, {2, 1}, {-1, 2}, {1, -2}};
    for (const auto& off : wideOffsets) {
        painter.drawImage(off[0], off[1], sceneImage);
    }
    painter.restore();

    // 3. CRT Horizontal scanlines
    painter.save();
    painter.setPen(QPen(QColor(0, 0, 0, 80), 1));
    for (int y = 0; y < height(); y += 2) {
        painter.drawLine(0, y, width(), y);
    }
    painter.restore();

    // 4. Curved tube radial vignette & bezel
    painter.save();
    const QPointF center(width() / 2.0, height() / 2.0);
    const qreal radius = std::max(width(), height()) * 0.75;
    QRadialGradient vignette(center, radius);
    vignette.setColorAt(0.0, QColor(0, 0, 0, 0));
    vignette.setColorAt(0.65, QColor(0, 0, 0, 0));
    vignette.setColorAt(0.85, QColor(0, 0, 0, 90));
    vignette.setColorAt(1.0, QColor(0, 0, 0, 210));
    painter.fillRect(rect(), QBrush(vignette));

    // Corner rounding bevel
    for (int c = 0; c < 14; ++c) {
        const int alpha = (14 - c) * 12;
        painter.setPen(QPen(QColor(0, 0, 0, alpha), 1));
        painter.drawLine(0, c, 14 - c, 0);
        painter.drawLine(width() - 1 - (14 - c), 0, width() - 1, c);
        painter.drawLine(0, height() - 1 - c, 14 - c, height() - 1);
        painter.drawLine(width() - 1 - (14 - c), height() - 1, width() - 1, height() - 1 - c);
    }
    painter.restore();
}

void QixCanvas::drawHud(QPainter& painter)
{
    const auto& theme = ColorPalette::get(m_paletteId);
    painter.save();

    // Top status bar background
    painter.fillRect(0, 0, width(), 45, toQColor(theme.hudBg));
    painter.setPen(QPen(toQColor(theme.hudBorder), 1));
    painter.drawLine(0, 45, width(), 45);

    QFont font("Monospace", 10, QFont::Bold);
    painter.setFont(font);

    const QColor labelColor = toQColor(theme.textLabel);
    const QColor valueColor = toQColor(theme.textValue);
    const QColor accentColor = toQColor(theme.textAccent);
    const QColor targetColor = toQColor(theme.progressBarTarget);
    const QColor livesColor = toQColor(theme.markerDiamond);

    // Score
    painter.setPen(labelColor);
    painter.drawText(15, 28, "SCORE:");
    painter.setPen(valueColor);
    painter.drawText(70, 28, QString::number(m_view.stats.score));

    // High Score
    painter.setPen(labelColor);
    painter.drawText(135, 28, "HIGH:");
    painter.setPen(valueColor);
    painter.drawText(180, 28, QString::number(m_view.stats.highScore));

    // Claimed Percentage
    painter.setPen(labelColor);
    painter.drawText(250, 28, "CLAIM:");
    const auto percent = m_view.stats.claimedPercent;
    const auto target = m_view.stats.targetPercent;
    painter.setPen(percent >= target ? targetColor : accentColor);
    painter.drawText(305, 28, QString("%1% / %2%").arg(percent).arg(target));

    // Progress Bar
    const int barX = 395;
    const int barY = 16;
    const int barW = 80;
    const int barH = 14;
    painter.setPen(Qt::NoPen);
    painter.fillRect(barX, barY, barW, barH, toQColor(theme.progressBarBg));
    const int fillW = std::min(barW, (barW * percent) / 100);
    painter.fillRect(barX, barY, fillW, barH, percent >= target ? targetColor : toQColor(theme.progressBarFill));

    // Lives
    painter.setPen(labelColor);
    painter.drawText(450, 28, "LIVES:");
    for (int i = 0; i < m_view.stats.lives; ++i) {
        painter.setBrush(livesColor);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(505 + i * 16, 18, 12, 12);
    }

    // Time
    const auto secondsRemaining = (m_view.stats.timeRemainingMs + 999U) / 1000U;
    const QColor timerColor = (m_view.stats.timeUp || secondsRemaining <= 10U)
        ? livesColor
        : ((secondsRemaining <= 20U) ? valueColor : targetColor);
    painter.setPen(labelColor);
    painter.drawText(width() - 365, 28, "TIME:");
    painter.setPen(timerColor);
    painter.drawText(width() - 320, 28, QString("%1s").arg(secondsRemaining));

    // Multiplier (if > 1) / Speed / Level
    if (m_view.stats.multiplier > 1) {
        painter.setPen(labelColor);
        painter.drawText(width() - 265, 28, "MULT:");
        painter.setPen(valueColor);
        painter.drawText(width() - 220, 28, QString("%1X").arg(m_view.stats.multiplier));

        painter.setPen(labelColor);
        painter.drawText(width() - 165, 28, "SPD:");
        painter.setPen(accentColor);
        painter.drawText(width() - 125, 28, QString("%1ms").arg(m_delayMs));

        painter.setPen(labelColor);
        painter.drawText(width() - 65, 28, "LVL:");
        painter.setPen(valueColor);
        painter.drawText(width() - 25, 28, QString::number(m_view.stats.level));
    } else {
        painter.setPen(labelColor);
        painter.drawText(width() - 200, 28, "SPEED:");
        painter.setPen(accentColor);
        painter.drawText(width() - 145, 28, QString("%1ms").arg(m_delayMs));

        painter.setPen(labelColor);
        painter.drawText(width() - 85, 28, "LEVEL:");
        painter.setPen(valueColor);
        painter.drawText(width() - 30, 28, QString::number(m_view.stats.level));
    }

    painter.restore();
}

void QixCanvas::drawPlayfield(QPainter& painter, const QRect& fieldRect)
{
    const auto gridW = m_view.playfield->getWidth();
    const auto gridH = m_view.playfield->getHeight();

    const double cellW = static_cast<double>(fieldRect.width()) / gridW;
    const double cellH = static_cast<double>(fieldRect.height()) / gridH;
    const auto& theme = ColorPalette::get(m_paletteId);

    if (m_artEnabled) {
        ensureArtImage();
    }

    const bool hasArt = m_artEnabled && !m_artImage.isNull();
    const int artW = hasArt ? m_artImage.width() : 0;
    const int artH = hasArt ? m_artImage.height() : 0;

    // Outer and interior cells
    for (std::int32_t y {0}; y < gridH; ++y) {
        for (std::int32_t x {0}; x < gridW; ++x) {
            const auto state = m_view.playfield->getCell(x, y);
            const QRectF r(fieldRect.left() + x * cellW, fieldRect.top() + y * cellH, cellW + 0.5, cellH + 0.5);

            if (state == CellState::Border) {
                painter.fillRect(r, toQColor(theme.playfieldBorder));
            } else if (state == CellState::ClaimedSlow) {
                if (hasArt) {
                    const int srcX = static_cast<int>((static_cast<double>(x) / gridW) * artW);
                    const int srcY = static_cast<int>((static_cast<double>(y) / gridH) * artH);
                    const int srcX2 = static_cast<int>((static_cast<double>(x + 1) / gridW) * artW);
                    const int srcY2 = static_cast<int>((static_cast<double>(y + 1) / gridH) * artH);
                    painter.drawImage(r, m_artImage, QRect(srcX, srcY, std::max(1, srcX2 - srcX), std::max(1, srcY2 - srcY)));
                } else {
                    painter.fillRect(r, toQColor(theme.claimedSlow));
                }
            } else if (state == CellState::ClaimedFast) {
                if (hasArt) {
                    const int srcX = static_cast<int>((static_cast<double>(x) / gridW) * artW);
                    const int srcY = static_cast<int>((static_cast<double>(y) / gridH) * artH);
                    const int srcX2 = static_cast<int>((static_cast<double>(x + 1) / gridW) * artW);
                    const int srcY2 = static_cast<int>((static_cast<double>(y + 1) / gridH) * artH);
                    painter.drawImage(r, m_artMutedImage, QRect(srcX, srcY, std::max(1, srcX2 - srcX), std::max(1, srcY2 - srcY)));
                } else {
                    painter.fillRect(r, toQColor(theme.claimedFast));
                }
            } else if (state == CellState::ActiveStix) {
                painter.fillRect(r, toQColor(theme.activeStix));
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
    const auto& theme = ColorPalette::get(m_paletteId);

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

            const int alpha = 255 - static_cast<int>((segIndex * 180) / std::max<std::size_t>(1, totalSegs));

            QColor lineColor;
            if (theme.ribbonMode == RibbonColorMode::NeonGradient) {
                const int hue = static_cast<int>((m_colorCycle * 4 + segIndex * 15) % 120 + 280) % 360;
                lineColor = QColor::fromHsv(hue, 230, 255, alpha);
            } else if (theme.ribbonMode == RibbonColorMode::MonochromeAmber) {
                const auto val = static_cast<int>(255 - (segIndex * 140) / std::max<std::size_t>(1, totalSegs));
                lineColor = QColor::fromHsv(38, 240, val, alpha);
            } else if (theme.ribbonMode == RibbonColorMode::MonochromeGreen) {
                const auto val = static_cast<int>(255 - (segIndex * 140) / std::max<std::size_t>(1, totalSegs));
                lineColor = QColor::fromHsv(142, 240, val, alpha);
            } else {
                const int hue = static_cast<int>(
                    (m_colorCycle * 5 + segIndex * (360 / std::max<std::size_t>(1, totalSegs))) % 360);
                lineColor = QColor::fromHsv(hue, 220, 255, alpha);
            }

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
    const auto& theme = ColorPalette::get(m_paletteId);

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
        painter.setPen(QPen(toQColor(theme.activeStix), 2.5));
        painter.drawPath(trailPath);
    }

    // 2. Sparx
    if (!m_view.sparxList.empty()) {
        for (const auto& sp : m_view.sparxList) {
            const double cx = fieldRect.left() + (sp.position.x + 0.5) * cellW;
            const double cy = fieldRect.top() + (sp.position.y + 0.5) * cellH;

            if (sp.isSuper) {
                // Super Sparx: bright cyan diamond with white center
                painter.setBrush(toQColor(theme.superSparx));
                painter.setPen(QPen(QColor(255, 255, 255), 1.5));
                QPolygonF diamond;
                diamond << QPointF(cx, cy - 7) << QPointF(cx + 7, cy) << QPointF(cx, cy + 7) << QPointF(cx - 7, cy);
                painter.drawPolygon(diamond);

                painter.setBrush(QColor(255, 255, 255));
                painter.setPen(Qt::NoPen);
                QPolygonF inner;
                inner << QPointF(cx, cy - 3) << QPointF(cx + 3, cy) << QPointF(cx, cy + 3) << QPointF(cx - 3, cy);
                painter.drawPolygon(inner);
            } else {
                // Glowing red/magenta diamond
                painter.setBrush(toQColor(theme.sparx));
                painter.setPen(QPen(QColor(255, 255, 255), 1.0));

                QPolygonF diamond;
                diamond << QPointF(cx, cy - 6) << QPointF(cx + 6, cy) << QPointF(cx, cy + 6) << QPointF(cx - 6, cy);
                painter.drawPolygon(diamond);
            }
        }
    } else {
        for (const auto& sp : m_view.sparxPositions) {
            const double cx = fieldRect.left() + (sp.x + 0.5) * cellW;
            const double cy = fieldRect.top() + (sp.y + 0.5) * cellH;

            painter.setBrush(toQColor(theme.sparx));
            painter.setPen(QPen(QColor(255, 255, 255), 1.0));

            QPolygonF diamond;
            diamond << QPointF(cx, cy - 6) << QPointF(cx + 6, cy) << QPointF(cx, cy + 6) << QPointF(cx - 6, cy);
            painter.drawPolygon(diamond);
        }
    }

    // 3. Fuse
    if (m_view.fusePos.has_value()) {
        const auto fp = m_view.fusePos.value();
        const double cx = fieldRect.left() + (fp.x + 0.5) * cellW;
        const double cy = fieldRect.top() + (fp.y + 0.5) * cellH;

        painter.setBrush(toQColor(theme.fuse));
        painter.setPen(QPen(toQColor(theme.textValue), 1.5));
        painter.drawEllipse(QPointF(cx, cy), 5, 5);
    }

    // 4. Player Marker
    const double mx = fieldRect.left() + (m_view.markerPos.x + 0.5) * cellW;
    const double my = fieldRect.top() + (m_view.markerPos.y + 0.5) * cellH;

    painter.setBrush(m_view.drawMode != DrawMode::None ? toQColor(theme.textValue) : toQColor(theme.marker));
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

    if (m_view.state == GameState::Attract) {
        if (m_view.attractStage == AttractStage::GameplayDemo) {
            drawDemoBanners(painter);
            return;
        }
    }

    painter.save();
    painter.fillRect(rect(), QColor(0, 0, 0, 200));

    if (m_view.state == GameState::Attract) {
        if (m_view.attractStage == AttractStage::TitleScores) {
            drawHallOfFame(painter, false, true);
        } else if (m_view.attractStage == AttractStage::Instructions) {
            drawInstructionsCard(painter);
        }
        painter.restore();
        return;
    }

    if (m_view.state == GameState::LevelComplete) {
        if (m_artEnabled && !m_artImage.isNull()) {
            const int margin = 20;
            const int hudHeight = 50;
            const QRect fieldRect(margin, hudHeight, width() - 2 * margin, height() - hudHeight - margin);
            painter.drawImage(fieldRect, m_artImage);

            const int bannerH = 160;
            const int bannerY = fieldRect.center().y() - bannerH / 2;
            painter.fillRect(fieldRect.left(), bannerY, fieldRect.width(), bannerH, QColor(0, 0, 0, 210));

            painter.setPen(QColor(250, 204, 21));
            painter.setFont(QFont("Monospace", 14, QFont::Bold));
            painter.drawText(QRect(fieldRect.left(), bannerY + 12, fieldRect.width(), 24), Qt::AlignCenter,
                QString("ART UNMASKED: %1").arg(BackgroundArt::getSceneName(m_currentArtScene)));
        }

        QFont font("Monospace", 22, QFont::Bold);
        painter.setFont(font);

        if (m_view.stats.qixTrapped) {
            painter.setPen(QColor(250, 204, 21));
            const QString title = m_view.stats.spiralBonus ? "*** SPIRAL QIX TRAP! ***" : "*** QIX TRAPPED! ***";
            painter.drawText(rect(), Qt::AlignCenter,
                QString("%1\nQIX CONFINED TO %2% OF FIELD!\n+%3 PTS TRAP BONUS!%4\nPress [Space] for Next Level")
                    .arg(title)
                    .arg(m_view.stats.qixRemainingPercent)
                    .arg(m_view.stats.trapBonus)
                    .arg(m_view.stats.thresholdBonus > 0 ? QString(" (+%1 OVERSHOOT)").arg(m_view.stats.thresholdBonus) : QString("")));
        } else if (m_view.stats.splitBonus) {
            painter.setPen(QColor(250, 204, 21));
            painter.drawText(rect(), Qt::AlignCenter,
                QString("QIX SPLIT BONUS!\nMultiplier: %1X\nPress [Space] for Next Level")
                    .arg(m_view.stats.multiplier));
        } else if (m_view.stats.thresholdBonus > 0) {
            painter.setPen(QColor(74, 222, 128));
            const auto overshoot = (m_view.stats.claimedPercent > m_view.stats.targetPercent)
                ? (m_view.stats.claimedPercent - m_view.stats.targetPercent)
                : 0;
            painter.drawText(rect(), Qt::AlignCenter,
                QString("LEVEL COMPLETE!\nTHRESHOLD BONUS: +%1 PTS (+%2%)\nPress [Space] for Next Level")
                    .arg(m_view.stats.thresholdBonus)
                    .arg(overshoot));
        } else {
            painter.setPen(QColor(74, 222, 128));
            painter.drawText(rect(), Qt::AlignCenter, "LEVEL COMPLETE!\nPress [Space] for Next Level");
        }
    } else if (m_view.state == GameState::NameEntry) {
        drawNameEntry(painter);
    } else if (m_view.state == GameState::HallOfFame) {
        drawHallOfFame(painter, false, false);
    } else if (m_view.state == GameState::GameOver) {
        drawHallOfFame(painter, true, false);
    }

    painter.restore();
}

void QixCanvas::drawNameEntry(QPainter& painter)
{
    const int w = width();
    const int h = height();

    painter.save();
    painter.setPen(QColor(250, 204, 21));
    painter.setFont(QFont("Monospace", 22, QFont::Bold));
    painter.drawText(QRect(0, h / 2 - 140, w, 40), Qt::AlignCenter, "ARCADE HALL OF FAME");

    painter.setPen(QColor(99, 179, 237));
    painter.setFont(QFont("Monospace", 14, QFont::Bold));
    painter.drawText(QRect(0, h / 2 - 95, w, 30), Qt::AlignCenter,
        QString("NEW HIGH SCORE! RANK #%1 - SCORE: %2").arg(m_view.nameEntry.rank).arg(m_view.stats.score));

    painter.setPen(QColor(243, 244, 246));
    painter.setFont(QFont("Monospace", 12));
    painter.drawText(QRect(0, h / 2 - 60, w, 25), Qt::AlignCenter, "ENTER YOUR INITIALS");

    // 3 letter boxes
    const int boxW = 54;
    const int boxH = 65;
    const int gap = 24;
    const int totalW = 3 * boxW + 2 * gap;
    const int startX = (w - totalW) / 2;
    const int boxY = h / 2 - 20;

    for (std::uint8_t i {0}; i < 3; ++i) {
        const int bx = startX + static_cast<int>(i) * (boxW + gap);
        const bool isActive = (m_view.nameEntry.cursorIndex == i);

        painter.fillRect(bx, boxY, boxW, boxH, QColor(15, 23, 42));
        painter.setPen(QPen(isActive ? QColor(250, 204, 21) : QColor(71, 85, 105), isActive ? 3 : 1));
        painter.drawRect(bx, boxY, boxW, boxH);

        if (isActive) {
            painter.setPen(QColor(250, 204, 21));
            painter.setFont(QFont("Monospace", 14, QFont::Bold));
            painter.drawText(QRect(bx, boxY - 20, boxW, 18), Qt::AlignCenter, "^");
            painter.drawText(QRect(bx, boxY + boxH + 2, boxW, 18), Qt::AlignCenter, "v");
        }

        painter.setPen(QColor(248, 250, 252));
        painter.setFont(QFont("Monospace", 26, QFont::Bold));
        const QString ch(m_view.nameEntry.initials[i]);
        painter.drawText(QRect(bx, boxY, boxW, boxH), Qt::AlignCenter, ch);
    }

    painter.setPen(QColor(148, 163, 184));
    painter.setFont(QFont("Monospace", 11));
    painter.drawText(
        QRect(0, h / 2 + 75, w, 25), Qt::AlignCenter, "[UP/DOWN] Letter   [LEFT/RIGHT] Slot   [ENTER/SPACE] Confirm");
    painter.restore();
}

void QixCanvas::drawHallOfFame(QPainter& painter, bool isGameOver, bool isAttract)
{
    const int w = width();
    const int h = height();

    painter.save();
    int curY = h / 2 - 160;

    if (isGameOver) {
        painter.setPen(QColor(248, 113, 113));
        painter.setFont(QFont("Monospace", 22, QFont::Bold));
        painter.drawText(QRect(0, curY, w, 35), Qt::AlignCenter, "GAME OVER");
        curY += 40;
    } else if (isAttract) {
        painter.setPen(QColor(59, 130, 246));
        painter.setFont(QFont("Monospace", 20, QFont::Bold));
        painter.drawText(QRect(0, curY, w, 35), Qt::AlignCenter, "TAITO 1981 - QIX ARCADE");
        curY += 40;
    }

    painter.setPen(QColor(250, 204, 21));
    painter.setFont(QFont("Monospace", 20, QFont::Bold));
    painter.drawText(QRect(0, curY, w, 32), Qt::AlignCenter, "ARCADE HALL OF FAME");
    curY += 38;

    painter.setPen(QColor(99, 179, 237));
    painter.setFont(QFont("Monospace", 12, QFont::Bold));
    const QString header = "RANK     NAME       SCORE      LVL    MODE";
    painter.drawText(QRect(0, curY, w, 22), Qt::AlignCenter, header);
    curY += 26;

    if (m_view.highScoreTable != nullptr) {
        const auto& entries = m_view.highScoreTable->getEntries();
        const std::size_t maxRows = std::min(entries.size(), static_cast<std::size_t>(8));

        painter.setFont(QFont("Monospace", 12));
        for (std::size_t i {0}; i < maxRows; ++i) {
            const auto& e = entries[i];
            QColor rowColor(148, 163, 184);
            if (i == 0) {
                rowColor = QColor(250, 204, 21);
            } else if (i == 1) {
                rowColor = QColor(226, 232, 240);
            } else if (i == 2) {
                rowColor = QColor(245, 158, 11);
            }

            painter.setPen(rowColor);
            const char* modeStr = (e.mode == GameMode::Classic) ? "CLASSIC" : "MODERN";
            const QString rowStr = QString("%1.      %-4s    %2      %3     %-7s")
                                       .arg(i + 1, 2)
                                       .arg(QString::fromStdString(e.initials))
                                       .arg(e.score, 8)
                                       .arg(e.level, 2, 10, QChar('0'))
                                       .arg(modeStr);
            painter.drawText(QRect(0, curY, w, 20), Qt::AlignCenter, rowStr);
            curY += 22;
        }
    }

    curY += 15;
    painter.setPen(isAttract ? QColor(74, 222, 128) : QColor(243, 244, 246));
    painter.setFont(QFont("Monospace", 12, QFont::Bold));
    painter.drawText(QRect(0, curY, w, 25), Qt::AlignCenter,
        isAttract ? "INSERT COIN  -  PRESS [SPACE] TO PLAY" : "Press [R] or [SPACE] to Play Again");
    painter.restore();
}

void QixCanvas::drawDemoBanners(QPainter& painter)
{
    const int w = width();
    const int h = height();

    painter.save();
    const QRect topBox((w - 340) / 2, 16, 340, 36);
    painter.fillRect(topBox, QColor(15, 23, 42, 220));
    painter.setPen(QPen(QColor(250, 204, 21), 2));
    painter.drawRect(topBox);
    painter.setFont(QFont("Monospace", 14, QFont::Bold));
    painter.setPen(QColor(250, 204, 21));
    painter.drawText(topBox, Qt::AlignCenter, "*** GAMEPLAY DEMO ***");

    const QRect botBox((w - 440) / 2, h - 50, 440, 34);
    painter.fillRect(botBox, QColor(15, 23, 42, 220));
    painter.setPen(QPen(QColor(74, 222, 128), 2));
    painter.drawRect(botBox);
    painter.setFont(QFont("Monospace", 12, QFont::Bold));
    painter.setPen(QColor(74, 222, 128));
    painter.drawText(botBox, Qt::AlignCenter, "INSERT COIN - PRESS ANY KEY TO PLAY");
    painter.restore();
}

void QixCanvas::drawInstructionsCard(QPainter& painter)
{
    const int w = width();
    const int h = height();

    painter.save();
    painter.setPen(QColor(250, 204, 21));
    painter.setFont(QFont("Monospace", 22, QFont::Bold));
    painter.drawText(QRect(0, h / 2 - 150, w, 35), Qt::AlignCenter, "HOW TO PLAY");

    struct Rule {
        const char* header;
        const char* detail;
        QColor color;
    };
    const std::array<Rule, 5> rules {{
        {"OBJECTIVE", "CLAIM 75% OR MORE OF THE PLAYFIELD TO WIN", QColor(59, 130, 246)},
        {"SLOW DRAW", "HOLD [SPACE] WHILE MOVING (2X POINTS - 200 PTS/CELL)", QColor(34, 197, 94)},
        {"FAST DRAW", "HOLD [SHIFT/F] WHILE MOVING (1X POINTS - 100 PTS/CELL)", QColor(245, 158, 11)},
        {"HAZARDS", "AVOID THE BOUNCING QIX & PATROLLING SPARX ENEMIES", QColor(239, 68, 68)},
        {"THE FUSE", "BURNS DOWN YOUR TRAIL IF YOU HESITATE - KEEP MOVING!", QColor(217, 70, 239)}
    }};

    int y = h / 2 - 95;
    for (const auto& r : rules) {
        painter.setPen(r.color);
        painter.setFont(QFont("Monospace", 11, QFont::Bold));
        painter.drawText(QRect(w / 2 - 250, y, 500, 20), Qt::AlignLeft, r.header);

        painter.setPen(QColor(229, 231, 235));
        painter.setFont(QFont("Monospace", 10));
        painter.drawText(QRect(w / 2 - 250, y + 18, 500, 20), Qt::AlignLeft, r.detail);
        y += 44;
    }

    painter.setPen(QColor(74, 222, 128));
    painter.setFont(QFont("Monospace", 12, QFont::Bold));
    painter.drawText(QRect(0, h / 2 + 145, w, 25), Qt::AlignCenter, "INSERT COIN - PRESS ANY KEY TO PLAY");
    painter.restore();
}

} // namespace qix::qt
