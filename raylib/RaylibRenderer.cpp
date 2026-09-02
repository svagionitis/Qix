#include "RaylibRenderer.h"
#include <algorithm>

namespace qix::raylib {

RaylibRenderer::~RaylibRenderer()
{
    if (m_initialized) {
        CloseWindow();
    }
}

bool RaylibRenderer::init(const std::string& title, int width, int height) noexcept
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
    InitWindow(width, height, title.c_str());
    m_initialized = IsWindowReady();
    return m_initialized;
}

bool RaylibRenderer::isInitialized() const noexcept
{
    return m_initialized;
}

void RaylibRenderer::render(const GameView& view, std::uint32_t delayMs) noexcept
{
    if (!m_initialized) {
        return;
    }

    BeginDrawing();

    // 1. Clear background (#0b0f19)
    ClearBackground(Color {11, 15, 25, 255});

    // 2. Render Top HUD Bar
    drawHud(view.stats, delayMs);

    // 3. Render Playfield
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();
    const float hudHeight = 50.0f;
    const float margin = 20.0f;

    const Rectangle fieldRect {margin, hudHeight, std::max(10.0f, static_cast<float>(screenW) - 2.0f * margin),
        std::max(10.0f, static_cast<float>(screenH) - hudHeight - margin)};

    if (view.playfield) {
        drawPlayfield(*view.playfield, fieldRect);
        drawQixRibbons(view.qixRibbons, fieldRect);
        drawEntities(view, fieldRect);
    }

    // 4. Overlays
    drawOverlays(view.state);

    EndDrawing();

    ++m_colorCycle;
}

void RaylibRenderer::drawHud(const GameStats& stats, std::uint32_t delayMs) noexcept
{
    const int screenW = GetScreenWidth();

    // Top status bar background (#121826)
    DrawRectangle(0, 0, screenW, 45, Color {18, 24, 38, 255});
    DrawLine(0, 45, screenW, 45, Color {35, 45, 68, 255});

    const Color labelColor {160, 174, 192, 255};
    const Color yellowColor {246, 224, 94, 255};
    const Color cyanColor {99, 179, 237, 255};
    const Color greenColor {72, 187, 120, 255};
    const Color purpleColor {183, 148, 244, 255};
    const Color speedColor {56, 178, 172, 255};
    const Color redColor {245, 101, 101, 255};

    const int fontSize = 16;
    const int textY = 15;

    // 1. SCORE
    DrawText("SCORE:", 20, textY, fontSize, labelColor);
    DrawText(std::to_string(stats.score).c_str(), 75, textY, fontSize, yellowColor);

    // 2. CLAIMED %
    DrawText("CLAIMED:", 175, textY, fontSize, labelColor);
    const std::string claimStr
        = std::to_string(stats.claimedPercent) + "% / " + std::to_string(stats.targetPercent) + "%";
    DrawText(
        claimStr.c_str(), 245, textY, fontSize, stats.claimedPercent >= stats.targetPercent ? greenColor : cyanColor);

    // 3. Progress Bar
    const int barX = 345;
    const int barY = 16;
    const int barW = 110;
    const int barH = 14;
    DrawRectangle(barX, barY, barW, barH, Color {30, 41, 59, 255});
    const int fillW = std::min(barW, (barW * static_cast<int>(stats.claimedPercent)) / 100);
    DrawRectangle(
        barX, barY, fillW, barH, stats.claimedPercent >= stats.targetPercent ? greenColor : Color {59, 130, 246, 255});

    // 4. LIVES
    DrawText("LIVES:", 475, textY, fontSize, labelColor);
    for (int i = 0; i < stats.lives; ++i) {
        DrawPoly(Vector2 {static_cast<float>(535 + i * 16), 23.0f}, 4, 6.0f, 45.0f, redColor);
    }

    // 5. SPEED
    DrawText("SPEED:", screenW - 200, textY, fontSize, labelColor);
    const std::string speedStr = std::to_string(delayMs) + "ms";
    DrawText(speedStr.c_str(), screenW - 145, textY, fontSize, speedColor);

    // 6. LEVEL
    DrawText("LEVEL:", screenW - 90, textY, fontSize, labelColor);
    DrawText(std::to_string(stats.level).c_str(), screenW - 35, textY, fontSize, purpleColor);
}

void RaylibRenderer::drawPlayfield(const Playfield& playfield, const Rectangle& fieldRect) noexcept
{
    const auto gridW = playfield.getWidth();
    const auto gridH = playfield.getHeight();
    if (gridW <= 0 || gridH <= 0) {
        return;
    }

    const float cellW = fieldRect.width / static_cast<float>(gridW);
    const float cellH = fieldRect.height / static_cast<float>(gridH);

    for (std::int32_t y {0}; y < gridH; ++y) {
        for (std::int32_t x {0}; x < gridW; ++x) {
            const auto state = playfield.getCell(x, y);
            if (state == CellState::Empty) {
                continue;
            }

            const Rectangle cellRect {fieldRect.x + static_cast<float>(x) * cellW,
                fieldRect.y + static_cast<float>(y) * cellH, cellW + 0.5f, cellH + 0.5f};

            if (state == CellState::Border) {
                DrawRectangleRec(cellRect, Color {59, 130, 246, 255});
            } else if (state == CellState::ClaimedSlow) {
                DrawRectangleRec(cellRect, Color {14, 116, 144, 200});
            } else if (state == CellState::ClaimedFast) {
                DrawRectangleRec(cellRect, Color {180, 83, 9, 200});
            } else if (state == CellState::ActiveStix) {
                DrawRectangleRec(cellRect, Color {255, 255, 255, 255});
            }
        }
    }
}

void RaylibRenderer::drawQixRibbons(
    const std::vector<std::deque<LineSegment>>& ribbons, const Rectangle& fieldRect) noexcept
{
    const float cellW = fieldRect.width / 80.0f;
    const float cellH = fieldRect.height / 60.0f;

    // Enable additive blending for glowing vector monitor aesthetics
    BeginBlendMode(BLEND_ADDITIVE);

    for (const auto& ribbon : ribbons) {
        if (ribbon.empty()) {
            continue;
        }

        const auto totalSegs = ribbon.size();
        std::size_t segIndex {0};

        for (const auto& seg : ribbon) {
            const Vector2 start {fieldRect.x + (static_cast<float>(seg.start.x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(seg.start.y) + 0.5f) * cellH};
            const Vector2 end {fieldRect.x + (static_cast<float>(seg.end.x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(seg.end.y) + 0.5f) * cellH};

            const float hue
                = static_cast<float>((m_colorCycle * 5 + segIndex * (360 / std::max<std::size_t>(1, totalSegs))) % 360);
            const auto alpha = static_cast<unsigned char>(
                255 - static_cast<int>((segIndex * 180) / std::max<std::size_t>(1, totalSegs)));

            Color lineColor = ColorFromHSV(hue, 0.85f, 1.0f);
            lineColor.a = alpha;

            DrawLineEx(start, end, (segIndex == 0) ? 3.0f : 2.0f, lineColor);

            ++segIndex;
        }
    }

    EndBlendMode();
}

void RaylibRenderer::drawEntities(const GameView& view, const Rectangle& fieldRect) noexcept
{
    const float cellW = fieldRect.width / 80.0f;
    const float cellH = fieldRect.height / 60.0f;

    // 1. Active Stix Trail
    if (view.stixTrail.size() >= 2) {
        for (std::size_t i = 1; i < view.stixTrail.size(); ++i) {
            const Vector2 p1 {fieldRect.x + (static_cast<float>(view.stixTrail[i - 1].x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(view.stixTrail[i - 1].y) + 0.5f) * cellH};
            const Vector2 p2 {fieldRect.x + (static_cast<float>(view.stixTrail[i].x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(view.stixTrail[i].y) + 0.5f) * cellH};
            DrawLineEx(p1, p2, 2.5f, WHITE);
        }
    }

    // 2. Sparx
    for (const auto& sp : view.sparxPositions) {
        const Vector2 center {fieldRect.x + (static_cast<float>(sp.x) + 0.5f) * cellW,
            fieldRect.y + (static_cast<float>(sp.y) + 0.5f) * cellH};
        DrawPoly(center, 4, 6.0f, 45.0f, Color {236, 72, 153, 255});
    }

    // 3. Fuse
    if (view.fusePos.has_value()) {
        const auto fp = view.fusePos.value();
        const Vector2 center {fieldRect.x + (static_cast<float>(fp.x) + 0.5f) * cellW,
            fieldRect.y + (static_cast<float>(fp.y) + 0.5f) * cellH};
        DrawCircleV(center, 5.0f, Color {254, 240, 138, 255});
        DrawCircleV(center, 3.0f, Color {239, 68, 68, 255});
    }

    // 4. Player Marker
    const Vector2 markerPos {fieldRect.x + (static_cast<float>(view.markerPos.x) + 0.5f) * cellW,
        fieldRect.y + (static_cast<float>(view.markerPos.y) + 0.5f) * cellH};
    const Color markerColor
        = (view.drawMode != DrawMode::None) ? Color {250, 204, 21, 255} : Color {243, 244, 246, 255};
    DrawPoly(markerPos, 4, 7.0f, 45.0f, markerColor);
}

void RaylibRenderer::drawOverlays(GameState state) noexcept
{
    if (state == GameState::Playing || state == GameState::Ready) {
        return;
    }

    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    // Semi-transparent blackout
    DrawRectangle(0, 0, screenW, screenH, Color {0, 0, 0, 180});

    if (state == GameState::LevelComplete) {
        const char* text1 = "LEVEL COMPLETE!";
        const char* text2 = "Press [Space] for Next Level";
        const int font1 = 28;
        const int font2 = 18;
        const int w1 = MeasureText(text1, font1);
        const int w2 = MeasureText(text2, font2);

        DrawText(text1, (screenW - w1) / 2, screenH / 2 - 30, font1, Color {74, 222, 128, 255});
        DrawText(text2, (screenW - w2) / 2, screenH / 2 + 15, font2, Color {243, 244, 246, 255});
    } else if (state == GameState::GameOver) {
        const char* text1 = "GAME OVER";
        const char* text2 = "Press [R] to Restart";
        const int font1 = 28;
        const int font2 = 18;
        const int w1 = MeasureText(text1, font1);
        const int w2 = MeasureText(text2, font2);

        DrawText(text1, (screenW - w1) / 2, screenH / 2 - 30, font1, Color {248, 113, 113, 255});
        DrawText(text2, (screenW - w2) / 2, screenH / 2 + 15, font2, Color {243, 244, 246, 255});
    }
}

} // namespace qix::raylib
