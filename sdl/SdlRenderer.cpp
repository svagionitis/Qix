#include "SdlRenderer.h"
#include <algorithm>
#include <cmath>

namespace qix::sdl {

bool SdlRenderer::init(const std::string& title, int width, int height) noexcept
{
    SDL_Window* window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (window == nullptr) {
        return false;
    }
    m_window.reset(window);

    SDL_Renderer* renderer = SDL_CreateRenderer(m_window.get(), -1, SDL_RENDERER_ACCELERATED);
    if (renderer == nullptr) {
        // Fallback to software renderer if hardware accelerated fails
        renderer = SDL_CreateRenderer(m_window.get(), -1, SDL_RENDERER_SOFTWARE);
        if (renderer == nullptr) {
            return false;
        }
    }
    m_renderer.reset(renderer);

    SDL_SetRenderDrawBlendMode(m_renderer.get(), SDL_BLENDMODE_BLEND);
    return true;
}

int SdlRenderer::getWidth() const noexcept
{
    if (!m_window) {
        return 0;
    }
    int w {0};
    int h {0};
    SDL_GetWindowSize(m_window.get(), &w, &h);
    return w;
}

int SdlRenderer::getHeight() const noexcept
{
    if (!m_window) {
        return 0;
    }
    int w {0};
    int h {0};
    SDL_GetWindowSize(m_window.get(), &w, &h);
    return h;
}

void SdlRenderer::render(const GameView& view, std::uint32_t delayMs) noexcept
{
    if (!m_renderer) {
        return;
    }

    const int screenW = getWidth();
    const int screenH = getHeight();

    // 1. Clear background (#0b0f19)
    SDL_SetRenderDrawColor(m_renderer.get(), 11, 15, 25, 255);
    SDL_RenderClear(m_renderer.get());

    // 2. Top HUD Bar
    drawHud(view.stats, delayMs);

    // 3. Compute Playfield Bounds
    const int hudHeight = 50;
    const int margin = 20;
    SDL_Rect fieldRect {
        margin, hudHeight, std::max(10, screenW - 2 * margin), std::max(10, screenH - hudHeight - margin)};

    if (view.playfield) {
        drawPlayfield(*view.playfield, fieldRect);
        drawQixRibbons(view.qixRibbons, fieldRect);
        drawEntities(view, fieldRect);
    }

    // 4. Overlays
    drawOverlays(view.state);

    ++m_colorCycle;
}

void SdlRenderer::present() noexcept
{
    if (m_renderer) {
        SDL_RenderPresent(m_renderer.get());
    }
}

void SdlRenderer::drawHud(const GameStats& stats, std::uint32_t delayMs) noexcept
{
    const int screenW = getWidth();

    // HUD background bar (#121826)
    SDL_Rect hudRect {0, 0, screenW, 45};
    SDL_SetRenderDrawColor(m_renderer.get(), 18, 24, 38, 255);
    SDL_RenderFillRect(m_renderer.get(), &hudRect);

    // Bottom border line (#232d44)
    SDL_SetRenderDrawColor(m_renderer.get(), 35, 45, 68, 255);
    SDL_RenderDrawLine(m_renderer.get(), 0, 45, screenW, 45);

    const SDL_Color labelColor {160, 174, 192, 255};
    const SDL_Color yellowColor {246, 224, 94, 255};
    const SDL_Color cyanColor {99, 179, 237, 255};
    const SDL_Color greenColor {72, 187, 120, 255};
    const SDL_Color purpleColor {183, 148, 244, 255};
    const SDL_Color speedColor {56, 178, 172, 255};
    const SDL_Color redColor {245, 101, 101, 255};

    // 1. SCORE
    BitmapFont::drawText(m_renderer.get(), "SCORE:", 20, 18, 1, labelColor);
    BitmapFont::drawText(m_renderer.get(), std::to_string(stats.score), 75, 18, 1, yellowColor);

    // 2. CLAIMED %
    BitmapFont::drawText(m_renderer.get(), "CLAIMED:", 175, 18, 1, labelColor);
    const std::string claimStr
        = std::to_string(stats.claimedPercent) + "% / " + std::to_string(stats.targetPercent) + "%";
    BitmapFont::drawText(
        m_renderer.get(), claimStr, 245, 18, 1, stats.claimedPercent >= stats.targetPercent ? greenColor : cyanColor);

    // 3. Progress Bar
    const int barX = 345;
    const int barY = 16;
    const int barW = 110;
    const int barH = 14;
    SDL_Rect bgBar {barX, barY, barW, barH};
    SDL_SetRenderDrawColor(m_renderer.get(), 30, 41, 59, 255);
    SDL_RenderFillRect(m_renderer.get(), &bgBar);

    const int fillW = std::min(barW, (barW * static_cast<int>(stats.claimedPercent)) / 100);
    SDL_Rect fillBar {barX, barY, fillW, barH};
    if (stats.claimedPercent >= stats.targetPercent) {
        SDL_SetRenderDrawColor(m_renderer.get(), greenColor.r, greenColor.g, greenColor.b, 255);
    } else {
        SDL_SetRenderDrawColor(m_renderer.get(), 59, 130, 246, 255);
    }
    SDL_RenderFillRect(m_renderer.get(), &fillBar);

    // 4. LIVES
    BitmapFont::drawText(m_renderer.get(), "LIVES:", 475, 18, 1, labelColor);
    for (int i = 0; i < stats.lives; ++i) {
        drawFilledDiamond(535 + i * 16, 22, 5, redColor);
    }

    // 5. SPEED
    BitmapFont::drawText(m_renderer.get(), "SPEED:", screenW - 200, 18, 1, labelColor);
    BitmapFont::drawText(m_renderer.get(), std::to_string(delayMs) + "ms", screenW - 145, 18, 1, speedColor);

    // 6. LEVEL
    BitmapFont::drawText(m_renderer.get(), "LEVEL:", screenW - 90, 18, 1, labelColor);
    BitmapFont::drawText(m_renderer.get(), std::to_string(stats.level), screenW - 35, 18, 1, purpleColor);
}

void SdlRenderer::drawPlayfield(const Playfield& playfield, const SDL_Rect& fieldRect) noexcept
{
    const auto gridW = playfield.getWidth();
    const auto gridH = playfield.getHeight();
    if (gridW <= 0 || gridH <= 0) {
        return;
    }

    const double cellW = static_cast<double>(fieldRect.w) / gridW;
    const double cellH = static_cast<double>(fieldRect.h) / gridH;

    for (std::int32_t y {0}; y < gridH; ++y) {
        for (std::int32_t x {0}; x < gridW; ++x) {
            const auto state = playfield.getCell(x, y);
            if (state == CellState::Empty) {
                continue;
            }

            SDL_Rect cellRect {static_cast<int>(fieldRect.x + x * cellW), static_cast<int>(fieldRect.y + y * cellH),
                static_cast<int>(cellW + 0.99), static_cast<int>(cellH + 0.99)};

            if (state == CellState::Border) {
                SDL_SetRenderDrawColor(m_renderer.get(), 59, 130, 246, 255);
                SDL_RenderFillRect(m_renderer.get(), &cellRect);
            } else if (state == CellState::ClaimedSlow) {
                SDL_SetRenderDrawColor(m_renderer.get(), 14, 116, 144, 200);
                SDL_RenderFillRect(m_renderer.get(), &cellRect);
            } else if (state == CellState::ClaimedFast) {
                SDL_SetRenderDrawColor(m_renderer.get(), 180, 83, 9, 200);
                SDL_RenderFillRect(m_renderer.get(), &cellRect);
            } else if (state == CellState::ActiveStix) {
                SDL_SetRenderDrawColor(m_renderer.get(), 255, 255, 255, 255);
                SDL_RenderFillRect(m_renderer.get(), &cellRect);
            }
        }
    }
}

void SdlRenderer::drawQixRibbons(
    const std::vector<std::deque<LineSegment>>& ribbons, const SDL_Rect& fieldRect) noexcept
{
    const double cellW = static_cast<double>(fieldRect.w) / 80.0;
    const double cellH = static_cast<double>(fieldRect.h) / 60.0;

    for (const auto& ribbon : ribbons) {
        if (ribbon.empty()) {
            continue;
        }

        const auto totalSegs = ribbon.size();
        std::size_t segIndex {0};

        for (const auto& seg : ribbon) {
            const int x1 = static_cast<int>(fieldRect.x + (seg.start.x + 0.5) * cellW);
            const int y1 = static_cast<int>(fieldRect.y + (seg.start.y + 0.5) * cellH);
            const int x2 = static_cast<int>(fieldRect.x + (seg.end.x + 0.5) * cellW);
            const int y2 = static_cast<int>(fieldRect.y + (seg.end.y + 0.5) * cellH);

            const int hue
                = static_cast<int>((m_colorCycle * 5 + segIndex * (360 / std::max<std::size_t>(1, totalSegs))) % 360);
            const auto alpha = static_cast<std::uint8_t>(
                255 - static_cast<int>((segIndex * 180) / std::max<std::size_t>(1, totalSegs)));

            const SDL_Color lineColor = hsvToRgb(hue, 0.85, 1.0, alpha);
            drawThickLine(x1, y1, x2, y2, (segIndex == 0) ? 3 : 2, lineColor);

            ++segIndex;
        }
    }
}

void SdlRenderer::drawEntities(const GameView& view, const SDL_Rect& fieldRect) noexcept
{
    const double cellW = static_cast<double>(fieldRect.w) / 80.0;
    const double cellH = static_cast<double>(fieldRect.h) / 60.0;

    // 1. Active Stix Trail
    if (view.stixTrail.size() >= 2) {
        SDL_SetRenderDrawColor(m_renderer.get(), 255, 255, 255, 255);
        for (std::size_t i = 1; i < view.stixTrail.size(); ++i) {
            const int x1 = static_cast<int>(fieldRect.x + (view.stixTrail[i - 1].x + 0.5) * cellW);
            const int y1 = static_cast<int>(fieldRect.y + (view.stixTrail[i - 1].y + 0.5) * cellH);
            const int x2 = static_cast<int>(fieldRect.x + (view.stixTrail[i].x + 0.5) * cellW);
            const int y2 = static_cast<int>(fieldRect.y + (view.stixTrail[i].y + 0.5) * cellH);
            drawThickLine(x1, y1, x2, y2, 2, SDL_Color {255, 255, 255, 255});
        }
    }

    // 2. Sparx
    for (const auto& sp : view.sparxPositions) {
        const int cx = static_cast<int>(fieldRect.x + (sp.x + 0.5) * cellW);
        const int cy = static_cast<int>(fieldRect.y + (sp.y + 0.5) * cellH);
        drawFilledDiamond(cx, cy, 6, SDL_Color {236, 72, 153, 255});
    }

    // 3. Fuse
    if (view.fusePos.has_value()) {
        const auto fp = view.fusePos.value();
        const int cx = static_cast<int>(fieldRect.x + (fp.x + 0.5) * cellW);
        const int cy = static_cast<int>(fieldRect.y + (fp.y + 0.5) * cellH);

        // Burning spark: outer yellow, inner red
        drawFilledDiamond(cx, cy, 6, SDL_Color {254, 240, 138, 255});
        drawFilledDiamond(cx, cy, 3, SDL_Color {239, 68, 68, 255});
    }

    // 4. Player Marker
    const int mx = static_cast<int>(fieldRect.x + (view.markerPos.x + 0.5) * cellW);
    const int my = static_cast<int>(fieldRect.y + (view.markerPos.y + 0.5) * cellH);

    const SDL_Color markerColor
        = (view.drawMode != DrawMode::None) ? SDL_Color {250, 204, 21, 255} : SDL_Color {243, 244, 246, 255};
    drawFilledDiamond(mx, my, 7, markerColor);
}

void SdlRenderer::drawOverlays(GameState state) noexcept
{
    if (state == GameState::Playing || state == GameState::Ready) {
        return;
    }

    const int screenW = getWidth();
    const int screenH = getHeight();

    // Semi-transparent blackout
    SDL_Rect fullScreen {0, 0, screenW, screenH};
    SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 180);
    SDL_RenderFillRect(m_renderer.get(), &fullScreen);

    if (state == GameState::LevelComplete) {
        const std::string line1 = "LEVEL COMPLETE!";
        const std::string line2 = "Press [Space] for Next Level";
        const int scale = 2;
        const int x1 = std::max(20, (screenW - static_cast<int>(line1.length()) * 8 * scale) / 2);
        const int y1 = screenH / 2 - 30;
        const int x2 = std::max(20, (screenW - static_cast<int>(line2.length()) * 8 * 1) / 2);
        const int y2 = screenH / 2 + 15;

        BitmapFont::drawText(m_renderer.get(), line1, x1, y1, scale, SDL_Color {74, 222, 128, 255});
        BitmapFont::drawText(m_renderer.get(), line2, x2, y2, 1, SDL_Color {243, 244, 246, 255});
    } else if (state == GameState::GameOver) {
        const std::string line1 = "GAME OVER";
        const std::string line2 = "Press [R] to Restart";
        const int scale = 2;
        const int x1 = std::max(20, (screenW - static_cast<int>(line1.length()) * 8 * scale) / 2);
        const int y1 = screenH / 2 - 30;
        const int x2 = std::max(20, (screenW - static_cast<int>(line2.length()) * 8 * 1) / 2);
        const int y2 = screenH / 2 + 15;

        BitmapFont::drawText(m_renderer.get(), line1, x1, y1, scale, SDL_Color {248, 113, 113, 255});
        BitmapFont::drawText(m_renderer.get(), line2, x2, y2, 1, SDL_Color {243, 244, 246, 255});
    }
}

void SdlRenderer::drawFilledDiamond(int cx, int cy, int radius, SDL_Color color) noexcept
{
    SDL_SetRenderDrawColor(m_renderer.get(), color.r, color.g, color.b, color.a);
    for (int dy = -radius; dy <= radius; ++dy) {
        const int span = radius - std::abs(dy);
        SDL_RenderDrawLine(m_renderer.get(), cx - span, cy + dy, cx + span, cy + dy);
    }
}

void SdlRenderer::drawThickLine(int x1, int y1, int x2, int y2, int thickness, SDL_Color color) noexcept
{
    SDL_SetRenderDrawColor(m_renderer.get(), color.r, color.g, color.b, color.a);
    SDL_RenderDrawLine(m_renderer.get(), x1, y1, x2, y2);
    if (thickness > 1) {
        SDL_RenderDrawLine(m_renderer.get(), x1 + 1, y1, x2 + 1, y2);
        SDL_RenderDrawLine(m_renderer.get(), x1 - 1, y1, x2 - 1, y2);
        SDL_RenderDrawLine(m_renderer.get(), x1, y1 + 1, x2, y2 + 1);
        SDL_RenderDrawLine(m_renderer.get(), x1, y1 - 1, x2, y2 - 1);
    }
}

SDL_Color SdlRenderer::hsvToRgb(int hue, double sat, double val, std::uint8_t alpha) noexcept
{
    hue = (hue % 360 + 360) % 360;
    const double c = val * sat;
    const double x = c * (1.0 - std::abs(std::fmod(hue / 60.0, 2.0) - 1.0));
    const double m = val - c;

    double r1 = 0;
    double g1 = 0;
    double b1 = 0;

    if (hue < 60) {
        r1 = c;
        g1 = x;
    } else if (hue < 120) {
        r1 = x;
        g1 = c;
    } else if (hue < 180) {
        g1 = c;
        b1 = x;
    } else if (hue < 240) {
        g1 = x;
        b1 = c;
    } else if (hue < 300) {
        r1 = x;
        b1 = c;
    } else {
        r1 = c;
        b1 = x;
    }

    return SDL_Color {static_cast<std::uint8_t>(std::clamp((r1 + m) * 255.0, 0.0, 255.0)),
        static_cast<std::uint8_t>(std::clamp((g1 + m) * 255.0, 0.0, 255.0)),
        static_cast<std::uint8_t>(std::clamp((b1 + m) * 255.0, 0.0, 255.0)), alpha};
}

} // namespace qix::sdl
