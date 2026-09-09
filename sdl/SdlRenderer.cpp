#include "SdlRenderer.h"
#include "GamePresenter.h"
#include "HighScoreTable.h"
#include "PlayfieldViewport.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
[[nodiscard]] constexpr SDL_Color toSdl(qix::PaletteColor c) noexcept
{
    return SDL_Color {c.r, c.g, c.b, c.a};
}
} // namespace

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

void SdlRenderer::setCrtEnabled(bool enabled) noexcept
{
    m_crtEnabled = enabled;
}

bool SdlRenderer::isCrtEnabled() const noexcept
{
    return m_crtEnabled;
}

void SdlRenderer::toggleCrt() noexcept
{
    m_crtEnabled = !m_crtEnabled;
}

void SdlRenderer::setPalette(PaletteId id) noexcept
{
    m_paletteId = id;
}

PaletteId SdlRenderer::getPalette() const noexcept
{
    return m_paletteId;
}

void SdlRenderer::cyclePalette() noexcept
{
    m_paletteId = ColorPalette::next(m_paletteId);
}

void SdlRenderer::setArtEnabled(bool enabled) noexcept
{
    m_artEnabled = enabled;
}

bool SdlRenderer::isArtEnabled() const noexcept
{
    return m_artEnabled;
}

void SdlRenderer::toggleArt() noexcept
{
    m_artEnabled = !m_artEnabled;
}

void SdlRenderer::setArtScene(int scene) noexcept
{
    m_forcedArtScene = scene;
}

void SdlRenderer::ensureArtTexture(ArtScene scene) noexcept
{
    if (m_artTexture && m_currentScene == scene) {
        return;
    }

    constexpr int ArtW = 320;
    constexpr int ArtH = 240;
    std::vector<std::uint8_t> buffer;
    BackgroundArt::generateRgbaBuffer(scene, ArtW, ArtH, buffer);

    SDL_Surface* surf
        = SDL_CreateRGBSurfaceWithFormatFrom(buffer.data(), ArtW, ArtH, 32, ArtW * 4, SDL_PIXELFORMAT_RGBA32);
    if (surf != nullptr) {
        SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer.get(), surf);
        SDL_FreeSurface(surf);
        if (tex != nullptr) {
            m_artTexture.reset(tex);
            m_artWidth = ArtW;
            m_artHeight = ArtH;
            m_currentScene = scene;
        }
    }
}

void SdlRenderer::render(const GameView& view, std::uint32_t delayMs) noexcept
{
    if (!m_renderer) {
        return;
    }

    const int screenW = getWidth();
    const int screenH = getHeight();
    const auto& theme = ColorPalette::get(m_paletteId);

    const auto activeScene = (m_forcedArtScene >= 0) ? BackgroundArt::fromIndex(m_forcedArtScene)
                                                     : BackgroundArt::getSceneForLevel(view.stats.level);

    if (m_artEnabled) {
        ensureArtTexture(activeScene);
    }

    if (m_crtEnabled) {
        if (!m_sceneTexture || m_textureWidth != screenW || m_textureHeight != screenH) {
            SDL_Texture* tex = SDL_CreateTexture(
                m_renderer.get(), SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, screenW, screenH);
            if (tex != nullptr) {
                SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
                m_sceneTexture.reset(tex);
                m_textureWidth = screenW;
                m_textureHeight = screenH;
            }
        }
        if (m_sceneTexture) {
            SDL_SetRenderTarget(m_renderer.get(), m_sceneTexture.get());
        }
    }

    // 1. Clear background
    SDL_SetRenderDrawColor(
        m_renderer.get(), theme.background.r, theme.background.g, theme.background.b, theme.background.a);
    SDL_RenderClear(m_renderer.get());

    // 2. Top HUD Bar
    drawHud(view.stats, delayMs);

    // 3. Compute Playfield Bounds
    const int hudHeight = 50;
    const int margin = 20;
    SDL_Rect fieldRect {
        margin, hudHeight, std::max(10, screenW - 2 * margin), std::max(10, screenH - hudHeight - margin)};

    // Particle simulation & event handling
    for (const auto& evt : view.events) {
        if (evt.type == GameEventType::MarkerDeath) {
            m_particles.emitMarkerExplosion(evt.position);
        } else if (evt.type == GameEventType::TerritoryCapture) {
            m_particles.emitCaptureFlash(evt.capturePerimeter, evt.drawMode);
        }
    }

    const std::uint64_t currentTicks = SDL_GetPerformanceCounter();
    const float dt = (m_lastFrameTicks == 0)
        ? 0.016f
        : static_cast<float>(currentTicks - m_lastFrameTicks) / static_cast<float>(SDL_GetPerformanceFrequency());
    m_lastFrameTicks = currentTicks;

    if (view.fusePos.has_value()) {
        m_particles.emitFuseSparkles(view.fusePos.value(), view.stixTrail, dt);
    }
    m_particles.update(dt);

    if (view.playfield) {
        drawPlayfield(*view.playfield, fieldRect, view);
        drawQixRibbons(view.qixRibbons, fieldRect);
        drawEntities(view, fieldRect);
        drawParticles(fieldRect);
    }

    // 4. Overlays
    drawOverlays(view, fieldRect);

    // 5. CRT Post-processing Filter (Scanlines, Phosphor Bloom & Vignette)
    if (m_crtEnabled && m_sceneTexture) {
        SDL_SetRenderTarget(m_renderer.get(), nullptr);
        applyCrtFilter(screenW, screenH);
    }

    ++m_colorCycle;
}

void SdlRenderer::applyCrtFilter(int width, int height) noexcept
{
    const auto& theme = ColorPalette::get(m_paletteId);
    // Clear backbuffer
    SDL_SetRenderDrawColor(
        m_renderer.get(), theme.crtBackdrop.r, theme.crtBackdrop.g, theme.crtBackdrop.b, theme.crtBackdrop.a);
    SDL_RenderClear(m_renderer.get());

    // 1. Draw base game scene
    SDL_RenderCopy(m_renderer.get(), m_sceneTexture.get(), nullptr, nullptr);

    // 2. Phosphor Glow / Bloom pass (additive blending with multi-tap spread)
    SDL_SetTextureBlendMode(m_sceneTexture.get(), SDL_BLENDMODE_ADD);
    SDL_SetTextureAlphaMod(m_sceneTexture.get(), 60);

    const int bloomOffsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
    for (const auto& offset : bloomOffsets) {
        const SDL_Rect bloomRect {offset[0], offset[1], width, height};
        SDL_RenderCopy(m_renderer.get(), m_sceneTexture.get(), nullptr, &bloomRect);
    }
    // Wider subtle halo
    SDL_SetTextureAlphaMod(m_sceneTexture.get(), 30);
    const int wideOffsets[4][2] = {{-2, -1}, {2, 1}, {-1, 2}, {1, -2}};
    for (const auto& offset : wideOffsets) {
        const SDL_Rect wideRect {offset[0], offset[1], width, height};
        SDL_RenderCopy(m_renderer.get(), m_sceneTexture.get(), nullptr, &wideRect);
    }

    // Restore texture settings
    SDL_SetTextureBlendMode(m_sceneTexture.get(), SDL_BLENDMODE_BLEND);
    SDL_SetTextureAlphaMod(m_sceneTexture.get(), 255);

    // 3. CRT Horizontal Scanlines
    SDL_SetRenderDrawBlendMode(m_renderer.get(), SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 80);
    for (int y = 0; y < height; y += 2) {
        SDL_RenderDrawLine(m_renderer.get(), 0, y, width, y);
    }

    // 4. Subtle CRT Vignette & Curved Bezel Shadow
    for (int b = 0; b < 10; ++b) {
        const auto alpha = static_cast<std::uint8_t>((10 - b) * 14);
        SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, alpha);
        SDL_RenderDrawLine(m_renderer.get(), 0, b, width, b);
        SDL_RenderDrawLine(m_renderer.get(), 0, height - 1 - b, width, height - 1 - b);
        SDL_RenderDrawLine(m_renderer.get(), b, 0, b, height);
        SDL_RenderDrawLine(m_renderer.get(), width - 1 - b, 0, width - 1 - b, height);
    }
    // Corner rounding bevel
    for (int c = 0; c < 14; ++c) {
        const auto alpha = static_cast<std::uint8_t>((14 - c) * 12);
        SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, alpha);
        SDL_RenderDrawLine(m_renderer.get(), 0, c, 14 - c, 0);
        SDL_RenderDrawLine(m_renderer.get(), width - 1 - (14 - c), 0, width - 1, c);
        SDL_RenderDrawLine(m_renderer.get(), 0, height - 1 - c, 14 - c, height - 1);
        SDL_RenderDrawLine(m_renderer.get(), width - 1 - (14 - c), height - 1, width - 1, height - 1 - c);
    }
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
    const auto& theme = ColorPalette::get(m_paletteId);

    // HUD background bar
    SDL_Rect hudRect {0, 0, screenW, 45};
    SDL_SetRenderDrawColor(m_renderer.get(), theme.hudBg.r, theme.hudBg.g, theme.hudBg.b, 255);
    SDL_RenderFillRect(m_renderer.get(), &hudRect);

    // Bottom border line
    SDL_SetRenderDrawColor(m_renderer.get(), theme.hudBorder.r, theme.hudBorder.g, theme.hudBorder.b, 255);
    SDL_RenderDrawLine(m_renderer.get(), 0, 45, screenW, 45);

    const SDL_Color labelColor = toSdl(theme.textLabel);
    const SDL_Color valueColor = toSdl(theme.textValue);
    const SDL_Color accentColor = toSdl(theme.textAccent);
    const SDL_Color greenColor = toSdl(theme.progressBarTarget);
    const SDL_Color redColor = toSdl(theme.markerDiamond);

    const auto hud = GamePresenter::formatHud(stats, delayMs);

    // 1. SCORE & HI
    BitmapFont::drawText(m_renderer.get(), "SCORE:", 15, 18, 1, labelColor);
    BitmapFont::drawText(m_renderer.get(), hud.scoreStr, 70, 18, 1, valueColor);
    BitmapFont::drawText(m_renderer.get(), "HI:", 135, 18, 1, labelColor);
    BitmapFont::drawText(m_renderer.get(), hud.hiScoreStr, 165, 18, 1, valueColor);

    // 2. CLAIMED %
    BitmapFont::drawText(m_renderer.get(), "CLAIM:", 240, 18, 1, labelColor);
    BitmapFont::drawText(m_renderer.get(), hud.claimStr, 295, 18, 1, hud.targetReached ? greenColor : accentColor);

    // 3. Progress Bar
    const int barX = 390;
    const int barY = 16;
    const int barW = 80;
    const int barH = 14;
    SDL_Rect bgBar {barX, barY, barW, barH};
    SDL_SetRenderDrawColor(m_renderer.get(), theme.progressBarBg.r, theme.progressBarBg.g, theme.progressBarBg.b, 255);
    SDL_RenderFillRect(m_renderer.get(), &bgBar);

    const int fillW = std::min(barW, (barW * hud.fillPercent) / 100);
    SDL_Rect fillBar {barX, barY, fillW, barH};
    if (hud.targetReached) {
        SDL_SetRenderDrawColor(m_renderer.get(), greenColor.r, greenColor.g, greenColor.b, 255);
    } else {
        SDL_SetRenderDrawColor(
            m_renderer.get(), theme.progressBarFill.r, theme.progressBarFill.g, theme.progressBarFill.b, 255);
    }
    SDL_RenderFillRect(m_renderer.get(), &fillBar);

    // 4. LIVES
    BitmapFont::drawText(m_renderer.get(), "LIVES:", 450, 18, 1, labelColor);
    for (int i = 0; i < stats.lives; ++i) {
        drawFilledDiamond(505 + i * 16, 22, 5, redColor);
    }

    // 5. TIME
    const SDL_Color timerColor = (hud.timeUrgency == HudUrgency::Critical)
        ? redColor
        : ((hud.timeUrgency == HudUrgency::Warning) ? valueColor : greenColor);
    BitmapFont::drawText(m_renderer.get(), "TIME:", screenW - 365, 18, 1, labelColor);
    BitmapFont::drawText(m_renderer.get(), hud.timeStr, screenW - 320, 18, 1, timerColor);

    // 6. MULTIPLIER / SPEED / LEVEL
    if (!hud.multiplierStr.empty()) {
        BitmapFont::drawText(m_renderer.get(), "MULT:", screenW - 265, 18, 1, labelColor);
        BitmapFont::drawText(m_renderer.get(), hud.multiplierStr, screenW - 220, 18, 1, valueColor);

        BitmapFont::drawText(m_renderer.get(), "SPD:", screenW - 165, 18, 1, labelColor);
        BitmapFont::drawText(m_renderer.get(), hud.speedStr, screenW - 125, 18, 1, accentColor);

        BitmapFont::drawText(m_renderer.get(), "LVL:", screenW - 65, 18, 1, labelColor);
        BitmapFont::drawText(m_renderer.get(), hud.levelStr, screenW - 25, 18, 1, valueColor);
    } else {
        BitmapFont::drawText(m_renderer.get(), "SPEED:", screenW - 200, 18, 1, labelColor);
        BitmapFont::drawText(m_renderer.get(), hud.speedStr, screenW - 145, 18, 1, accentColor);

        BitmapFont::drawText(m_renderer.get(), "LEVEL:", screenW - 85, 18, 1, labelColor);
        BitmapFont::drawText(m_renderer.get(), hud.levelStr, screenW - 30, 18, 1, valueColor);
    }
}

void SdlRenderer::drawPlayfield(const Playfield& playfield, const SDL_Rect& fieldRect, const GameView& view) noexcept
{
    (void)view;
    const auto gridW = playfield.getWidth();
    const auto gridH = playfield.getHeight();
    if (gridW <= 0 || gridH <= 0) {
        return;
    }

    const auto& theme = ColorPalette::get(m_paletteId);
    const PlayfieldViewport vp {static_cast<float>(fieldRect.x), static_cast<float>(fieldRect.y),
        static_cast<float>(fieldRect.w), static_cast<float>(fieldRect.h), gridW, gridH};

    for (std::int32_t y {0}; y < gridH; ++y) {
        for (std::int32_t x {0}; x < gridW; ++x) {
            const auto state = playfield.getCell(x, y);
            if (state == CellState::Empty) {
                continue;
            }

            const auto vr = vp.cellToScreenPixel(x, y);
            SDL_Rect cellRect {vr.x, vr.y, vr.width, vr.height};

            if (state == CellState::ClaimedSlow || state == CellState::ClaimedFast) {
                if (m_artEnabled && m_artTexture) {
                    const auto sr = vp.cellToTextureSrc(x, y, m_artWidth, m_artHeight);
                    const SDL_Rect srcRect {sr.x, sr.y, sr.width, sr.height};
                    if (state == CellState::ClaimedSlow) {
                        SDL_SetTextureColorMod(m_artTexture.get(), 255, 255, 255);
                    } else {
                        // Fast draw: cool cyan/blue tint
                        SDL_SetTextureColorMod(m_artTexture.get(), 180, 220, 255);
                    }
                    SDL_RenderCopy(m_renderer.get(), m_artTexture.get(), &srcRect, &cellRect);
                } else {
                    const auto& c = (state == CellState::ClaimedSlow) ? theme.claimedSlow : theme.claimedFast;
                    SDL_SetRenderDrawColor(m_renderer.get(), c.r, c.g, c.b, c.a);
                    SDL_RenderFillRect(m_renderer.get(), &cellRect);
                }
            } else if (state == CellState::Border) {
                // Fill the half of the border cell facing any claimed neighbor so claimed territory
                // meets the thin vector line seamlessly without gaps
                const double cellW = static_cast<double>(fieldRect.w) / static_cast<double>(gridW);
                const double cellH = static_cast<double>(fieldRect.h) / static_cast<double>(gridH);
                const int cx = static_cast<int>(fieldRect.x + (x + 0.5) * cellW);
                const int cy = static_cast<int>(fieldRect.y + (y + 0.5) * cellH);

                auto fillHalf = [&](int dx, int dy, CellState neighborState) {
                    const auto& c = (neighborState == CellState::ClaimedSlow) ? theme.claimedSlow : theme.claimedFast;
                    SDL_SetRenderDrawColor(m_renderer.get(), c.r, c.g, c.b, c.a);
                    SDL_Rect subRect {};
                    if (dx > 0) {
                        subRect = SDL_Rect {cx, vr.y, vr.width - (cx - vr.x), vr.height};
                    } else if (dx < 0) {
                        subRect = SDL_Rect {vr.x, vr.y, cx - vr.x + 1, vr.height};
                    } else if (dy > 0) {
                        subRect = SDL_Rect {vr.x, cy, vr.width, vr.height - (cy - vr.y)};
                    } else if (dy < 0) {
                        subRect = SDL_Rect {vr.x, vr.y, vr.width, cy - vr.y + 1};
                    }
                    SDL_RenderFillRect(m_renderer.get(), &subRect);
                };

                if (x + 1 < gridW) {
                    const auto s = playfield.getCell(x + 1, y);
                    if (s == CellState::ClaimedSlow || s == CellState::ClaimedFast) {
                        fillHalf(1, 0, s);
                    }
                }
                if (x > 0) {
                    const auto s = playfield.getCell(x - 1, y);
                    if (s == CellState::ClaimedSlow || s == CellState::ClaimedFast) {
                        fillHalf(-1, 0, s);
                    }
                }
                if (y + 1 < gridH) {
                    const auto s = playfield.getCell(x, y + 1);
                    if (s == CellState::ClaimedSlow || s == CellState::ClaimedFast) {
                        fillHalf(0, 1, s);
                    }
                }
                if (y > 0) {
                    const auto s = playfield.getCell(x, y - 1);
                    if (s == CellState::ClaimedSlow || s == CellState::ClaimedFast) {
                        fillHalf(0, -1, s);
                    }
                }
            }
        }
    }

    // Render slender 1-pixel vector lines connecting adjacent border cells
    const double cellW = static_cast<double>(fieldRect.w) / static_cast<double>(gridW);
    const double cellH = static_cast<double>(fieldRect.h) / static_cast<double>(gridH);
    for (std::int32_t y {0}; y < gridH; ++y) {
        for (std::int32_t x {0}; x < gridW; ++x) {
            if (playfield.getCell(x, y) != CellState::Border) {
                continue;
            }
            const int c1x = static_cast<int>(fieldRect.x + (x + 0.5) * cellW);
            const int c1y = static_cast<int>(fieldRect.y + (y + 0.5) * cellH);

            if (x + 1 < gridW && playfield.getCell(x + 1, y) == CellState::Border) {
                const int c2x = static_cast<int>(fieldRect.x + (x + 1.5) * cellW);
                drawThickLine(c1x, c1y, c2x, c1y, 1, toSdl(theme.playfieldBorder));
            }
            if (y + 1 < gridH && playfield.getCell(x, y + 1) == CellState::Border) {
                const int c2y = static_cast<int>(fieldRect.y + (y + 1.5) * cellH);
                drawThickLine(c1x, c1y, c1x, c2y, 1, toSdl(theme.playfieldBorder));
            }
        }
    }
}

void SdlRenderer::drawQixRibbons(
    const std::vector<std::deque<LineSegment>>& ribbons, const SDL_Rect& fieldRect) noexcept
{
    const PlayfieldViewport vp {static_cast<float>(fieldRect.x), static_cast<float>(fieldRect.y),
        static_cast<float>(fieldRect.w), static_cast<float>(fieldRect.h), 80, 60};
    const double cellW = vp.cellWidth();
    const double cellH = vp.cellHeight();
    const auto& theme = ColorPalette::get(m_paletteId);

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

            const auto col = ColorPalette::computeRibbonColor(theme, m_colorCycle, segIndex, totalSegs);
            drawThickLine(x1, y1, x2, y2, (segIndex == 0) ? 3 : 2, toSdl(col));

            ++segIndex;
        }
    }
}

void SdlRenderer::drawEntities(const GameView& view, const SDL_Rect& fieldRect) noexcept
{
    const double cellW = static_cast<double>(fieldRect.w) / 80.0;
    const double cellH = static_cast<double>(fieldRect.h) / 60.0;
    const auto& theme = ColorPalette::get(m_paletteId);

    // 1. Active Stix Trail
    if (view.stixTrail.size() >= 2) {
        SDL_SetRenderDrawColor(
            m_renderer.get(), theme.activeStix.r, theme.activeStix.g, theme.activeStix.b, theme.activeStix.a);
        for (std::size_t i = 1; i < view.stixTrail.size(); ++i) {
            const int x1 = static_cast<int>(fieldRect.x + (view.stixTrail[i - 1].x + 0.5) * cellW);
            const int y1 = static_cast<int>(fieldRect.y + (view.stixTrail[i - 1].y + 0.5) * cellH);
            const int x2 = static_cast<int>(fieldRect.x + (view.stixTrail[i].x + 0.5) * cellW);
            const int y2 = static_cast<int>(fieldRect.y + (view.stixTrail[i].y + 0.5) * cellH);
            drawThickLine(x1, y1, x2, y2, 1, toSdl(theme.activeStix));
        }
    }

    // 2. Sparx
    if (!view.sparxList.empty()) {
        for (const auto& sp : view.sparxList) {
            const int cx = static_cast<int>(fieldRect.x + (sp.position.x + 0.5) * cellW);
            const int cy = static_cast<int>(fieldRect.y + (sp.position.y + 0.5) * cellH);
            if (sp.isSuper) {
                drawFilledDiamond(cx, cy, 7, toSdl(theme.superSparx));
                drawFilledDiamond(cx, cy, 4, SDL_Color {255, 255, 255, 255});
            } else {
                drawFilledDiamond(cx, cy, 6, toSdl(theme.sparx));
            }
        }
    } else {
        for (const auto& sp : view.sparxPositions) {
            const int cx = static_cast<int>(fieldRect.x + (sp.x + 0.5) * cellW);
            const int cy = static_cast<int>(fieldRect.y + (sp.y + 0.5) * cellH);
            drawFilledDiamond(cx, cy, 6, toSdl(theme.sparx));
        }
    }

    // 3. Fuse
    if (view.fusePos.has_value()) {
        const auto fp = view.fusePos.value();
        const int cx = static_cast<int>(fieldRect.x + (fp.x + 0.5) * cellW);
        const int cy = static_cast<int>(fieldRect.y + (fp.y + 0.5) * cellH);

        drawFilledDiamond(cx, cy, 6, toSdl(theme.textValue));
        drawFilledDiamond(cx, cy, 3, toSdl(theme.fuse));
    }

    // 4. Player Marker
    const int mx = static_cast<int>(fieldRect.x + (view.markerPos.x + 0.5) * cellW);
    const int my = static_cast<int>(fieldRect.y + (view.markerPos.y + 0.5) * cellH);

    const SDL_Color markerColor = (view.drawMode != DrawMode::None) ? toSdl(theme.textValue) : toSdl(theme.marker);
    drawFilledDiamond(mx, my, 7, markerColor);
}

void SdlRenderer::drawParticles(const SDL_Rect& fieldRect) noexcept
{
    if (m_particles.empty()) {
        return;
    }

    const float cellW = static_cast<float>(fieldRect.w) / 80.0f;
    const float cellH = static_cast<float>(fieldRect.h) / 60.0f;
    const float fx = static_cast<float>(fieldRect.x);
    const float fy = static_cast<float>(fieldRect.y);
    const auto* parts = m_particles.data();
    const std::size_t count = m_particles.size();

    SDL_SetRenderDrawBlendMode(m_renderer.get(), SDL_BLENDMODE_BLEND);

    for (std::size_t i = 0; i < count; ++i) {
        const auto& p = parts[i];
        const int px = static_cast<int>(fx + p.x * cellW);
        const int py = static_cast<int>(fy + p.y * cellH);
        const auto col = p.currentColor();
        const SDL_Color sdlCol = toSdl(col);
        const int rad = std::max(1, static_cast<int>(p.size * cellW));

        switch (p.type) {
        case ParticleType::Spark: {
            const float speed = std::hypot(p.vx, p.vy);
            const float trailLen = std::clamp(speed * 0.04f * cellW, 2.0f, 12.0f);
            const float normVx = (speed > 0.001f) ? (p.vx / speed) : 0.0f;
            const float normVy = (speed > 0.001f) ? (p.vy / speed) : 0.0f;
            const int p2x = static_cast<int>(px - normVx * trailLen);
            const int p2y = static_cast<int>(py - normVy * trailLen);
            drawThickLine(px, py, p2x, p2y, std::max(1, rad), sdlCol);
            break;
        }
        case ParticleType::GlowShard:
        case ParticleType::DebrisDiamond: {
            drawFilledDiamond(px, py, rad, sdlCol);
            break;
        }
        case ParticleType::DebrisSquare: {
            SDL_SetRenderDrawColor(m_renderer.get(), sdlCol.r, sdlCol.g, sdlCol.b, sdlCol.a);
            const SDL_Rect r {px - rad, py - rad, rad * 2 + 1, rad * 2 + 1};
            SDL_RenderFillRect(m_renderer.get(), &r);
            break;
        }
        case ParticleType::DebrisLine: {
            const float halfLen = static_cast<float>(rad) * 1.6f;
            const float cosR = std::cos(p.rotation);
            const float sinR = std::sin(p.rotation);
            const int x1 = static_cast<int>(px - cosR * halfLen);
            const int y1 = static_cast<int>(py - sinR * halfLen);
            const int x2 = static_cast<int>(px + cosR * halfLen);
            const int y2 = static_cast<int>(py + sinR * halfLen);
            drawThickLine(x1, y1, x2, y2, 2, sdlCol);
            break;
        }
        }
    }
}

void SdlRenderer::drawOverlays(const GameView& view, const SDL_Rect& fieldRect) noexcept
{
    if (view.state == GameState::Playing || view.state == GameState::Ready) {
        return;
    }

    if (view.state == GameState::Attract) {
        if (view.attractStage == AttractStage::GameplayDemo) {
            drawDemoBanners();
            return;
        }
    }

    const int screenW = getWidth();
    const int screenH = getHeight();

    if (view.state == GameState::LevelComplete && m_artEnabled && m_artTexture) {
        // Grand victory curtain reveal: full background art unmasked!
        SDL_SetTextureColorMod(m_artTexture.get(), 255, 255, 255);
        SDL_RenderCopy(m_renderer.get(), m_artTexture.get(), nullptr, &fieldRect);
        SDL_SetRenderDrawColor(m_renderer.get(), 250, 204, 21, 255);
        SDL_RenderDrawRect(m_renderer.get(), &fieldRect);

        // Soft banner backdrop behind text
        SDL_SetRenderDrawBlendMode(m_renderer.get(), SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 190);
        SDL_Rect bannerBox {0, screenH / 2 - 95, screenW, 190};
        SDL_RenderFillRect(m_renderer.get(), &bannerBox);
    } else {
        // Semi-transparent blackout
        SDL_Rect fullScreen {0, 0, screenW, screenH};
        SDL_SetRenderDrawBlendMode(m_renderer.get(), SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(m_renderer.get(), 0, 0, 0, 200);
        SDL_RenderFillRect(m_renderer.get(), &fullScreen);
    }

    if (view.state == GameState::Attract) {
        if (view.attractStage == AttractStage::TitleScores) {
            drawHallOfFame(view.highScoreTable, false, true);
        } else if (view.attractStage == AttractStage::Instructions) {
            drawInstructionsCard();
        }
        return;
    }

    if (view.state == GameState::LevelComplete) {
        if (m_artEnabled && m_artTexture) {
            const std::string sceneBanner = std::string("ART UNMASKED: ") + BackgroundArt::getSceneName(m_currentScene);
            const int xs = std::max(20, (screenW - static_cast<int>(sceneBanner.length()) * 8 * 1) / 2);
            BitmapFont::drawText(m_renderer.get(), sceneBanner, xs, screenH / 2 - 80, 1, SDL_Color {255, 215, 0, 255});
        }
        const auto pres = GamePresenter::formatVictory(view.stats);
        const int scale = 2;
        const int x1 = std::max(20, (screenW - static_cast<int>(pres.title.length()) * 8 * scale) / 2);
        const int y1 = pres.isTrap ? (screenH / 2 - 50) : (screenH / 2 - 35);
        BitmapFont::drawText(m_renderer.get(), pres.title, x1, y1, scale, toSdl(pres.titleColor));

        if (pres.hasDetail) {
            const int xd = std::max(20, (screenW - static_cast<int>(pres.detail.length()) * 8 * 1) / 2);
            const int yd = screenH / 2 - 20;
            BitmapFont::drawText(m_renderer.get(), pres.detail, xd, yd, 1, SDL_Color {56, 189, 248, 255});
        }

        if (pres.hasBonus) {
            const int xb = std::max(20, (screenW - static_cast<int>(pres.bonus.length()) * 8 * 1) / 2);
            const int yb = pres.isTrap ? (screenH / 2 + 5) : (screenH / 2);
            BitmapFont::drawText(m_renderer.get(), pres.bonus, xb, yb, 1, SDL_Color {250, 204, 21, 255});
        }

        const int x2 = std::max(20, (screenW - static_cast<int>(pres.prompt.length()) * 8 * 1) / 2);
        const int y2 = pres.isTrap
            ? (screenH / 2 + 30)
            : ((!view.stats.splitBonus && pres.hasBonus) ? (screenH / 2 + 25) : (screenH / 2 + 15));
        BitmapFont::drawText(m_renderer.get(), pres.prompt, x2, y2, 1, SDL_Color {243, 244, 246, 255});
    } else if (view.state == GameState::NameEntry) {
        drawNameEntry(view.nameEntry, view.stats);
    } else if (view.state == GameState::HallOfFame) {
        drawHallOfFame(view.highScoreTable, false, false);
    } else if (view.state == GameState::GameOver) {
        drawHallOfFame(view.highScoreTable, true, false);
    }
}

void SdlRenderer::drawNameEntry(const NameEntryState& entry, const GameStats& stats) noexcept
{
    const int screenW = getWidth();
    const int screenH = getHeight();

    const std::string title = "ARCADE HALL OF FAME";
    const int scaleTitle = 2;
    const int xTitle = std::max(10, (screenW - static_cast<int>(title.length()) * 8 * scaleTitle) / 2);
    BitmapFont::drawText(m_renderer.get(), title, xTitle, screenH / 2 - 130, scaleTitle, SDL_Color {250, 204, 21, 255});

    const auto rankStr = GamePresenter::formatRecordBanner(entry.rank, stats.score);
    const int xRank = std::max(10, (screenW - static_cast<int>(rankStr.length()) * 8 * 1) / 2);
    BitmapFont::drawText(m_renderer.get(), rankStr, xRank, screenH / 2 - 90, 1, SDL_Color {99, 179, 237, 255});

    const std::string prompt = "ENTER YOUR INITIALS";
    const int xPrompt = std::max(10, (screenW - static_cast<int>(prompt.length()) * 8 * 1) / 2);
    BitmapFont::drawText(m_renderer.get(), prompt, xPrompt, screenH / 2 - 60, 1, SDL_Color {243, 244, 246, 255});

    // 3 Letter Boxes
    const int boxW = 50;
    const int boxH = 60;
    const int gap = 20;
    const int totalW = 3 * boxW + 2 * gap;
    const int startX = (screenW - totalW) / 2;
    const int boxY = screenH / 2 - 20;

    for (std::uint8_t i {0}; i < 3; ++i) {
        const int bx = startX + static_cast<int>(i) * (boxW + gap);
        const bool isActive = (entry.cursorIndex == i);

        SDL_Rect rect {bx, boxY, boxW, boxH};
        SDL_SetRenderDrawColor(m_renderer.get(), 15, 23, 42, 255);
        SDL_RenderFillRect(m_renderer.get(), &rect);

        const SDL_Color borderCol = isActive ? SDL_Color {250, 204, 21, 255} : SDL_Color {71, 85, 105, 255};
        SDL_SetRenderDrawColor(m_renderer.get(), borderCol.r, borderCol.g, borderCol.b, borderCol.a);
        SDL_RenderDrawRect(m_renderer.get(), &rect);

        if (isActive) {
            BitmapFont::drawText(m_renderer.get(), "^", bx + boxW / 2 - 4, boxY - 14, 1, SDL_Color {250, 204, 21, 255});
            BitmapFont::drawText(
                m_renderer.get(), "v", bx + boxW / 2 - 4, boxY + boxH + 4, 1, SDL_Color {250, 204, 21, 255});
        }

        const std::string letterStr(1, entry.initials[i]);
        BitmapFont::drawText(
            m_renderer.get(), letterStr, bx + boxW / 2 - 8, boxY + 18, 2, SDL_Color {248, 250, 252, 255});
    }

    const std::string navHelp = "[UP/DOWN] Letter   [LEFT/RIGHT] Slot   [ENTER/SPACE] Confirm";
    const int xHelp = std::max(10, (screenW - static_cast<int>(navHelp.length()) * 8 * 1) / 2);
    BitmapFont::drawText(m_renderer.get(), navHelp, xHelp, screenH / 2 + 75, 1, SDL_Color {148, 163, 184, 255});
}

void SdlRenderer::drawHallOfFame(const HighScoreTable* table, bool isGameOver, bool isAttract) noexcept
{
    const int screenW = getWidth();
    const int screenH = getHeight();

    int curY = screenH / 2 - 160;

    if (isGameOver) {
        const std::string go = "GAME OVER";
        const int xGo = std::max(10, (screenW - static_cast<int>(go.length()) * 8 * 2) / 2);
        BitmapFont::drawText(m_renderer.get(), go, xGo, curY, 2, SDL_Color {248, 113, 113, 255});
        curY += 35;
    } else if (isAttract) {
        const std::string att = "TAITO 1981 - QIX ARCADE";
        const int xAtt = std::max(10, (screenW - static_cast<int>(att.length()) * 8 * 1) / 2);
        BitmapFont::drawText(m_renderer.get(), att, xAtt, curY, 1, SDL_Color {59, 130, 246, 255});
        curY += 25;
    }

    const std::string title = "*** ARCADE HALL OF FAME ***";
    const int xTitle = std::max(10, (screenW - static_cast<int>(title.length()) * 8 * 1) / 2);
    BitmapFont::drawText(m_renderer.get(), title, xTitle, curY, 1, SDL_Color {250, 204, 21, 255});
    curY += 25;

    const std::string header = "RANK     NAME       SCORE      LVL    MODE";
    const int xHeader = std::max(10, (screenW - static_cast<int>(header.length()) * 8 * 1) / 2);
    BitmapFont::drawText(m_renderer.get(), header, xHeader, curY, 1, SDL_Color {99, 179, 237, 255});
    curY += 14;

    SDL_SetRenderDrawColor(m_renderer.get(), 51, 65, 85, 255);
    SDL_RenderDrawLine(m_renderer.get(), xHeader, curY, xHeader + static_cast<int>(header.length()) * 8, curY);
    curY += 6;

    if (table != nullptr) {
        const auto rows = GamePresenter::formatHallOfFame(table, 7);
        for (const auto& r : rows) {
            const int xRow = std::max(10, (screenW - static_cast<int>(r.formattedRow.length()) * 8 * 1) / 2);
            BitmapFont::drawText(m_renderer.get(), r.formattedRow, xRow, curY, 1, toSdl(r.medalColor));
            curY += 16;
        }
    }

    curY += 15;
    const std::uint32_t ticks = SDL_GetTicks();
    const bool blink = PlayfieldViewport::isBlinkOn(ticks, 350);
    const auto prompt = GamePresenter::formatHofPrompt(isAttract, blink);
    if (!prompt.empty()) {
        const int xPrompt = std::max(10, (screenW - static_cast<int>(prompt.length()) * 8 * 1) / 2);
        BitmapFont::drawText(m_renderer.get(), prompt, xPrompt, curY, 1,
            isAttract ? SDL_Color {74, 222, 128, 255} : SDL_Color {243, 244, 246, 255});
    }
}

void SdlRenderer::drawDemoBanners() noexcept
{
    const int screenW = getWidth();
    const int screenH = getHeight();

    // Top banner card
    const std::string demoText = "*** GAMEPLAY DEMO ***";
    const int xDemo = std::max(10, (screenW - static_cast<int>(demoText.length()) * 8 * 1) / 2);

    SDL_Rect topBox {xDemo - 16, 12, static_cast<int>(demoText.length()) * 8 + 32, 28};
    SDL_SetRenderDrawColor(m_renderer.get(), 15, 23, 42, 220);
    SDL_RenderFillRect(m_renderer.get(), &topBox);
    SDL_SetRenderDrawColor(m_renderer.get(), 250, 204, 21, 255);
    SDL_RenderDrawRect(m_renderer.get(), &topBox);

    BitmapFont::drawText(m_renderer.get(), demoText, xDemo, 20, 1, SDL_Color {250, 204, 21, 255});

    // Bottom banner
    const std::uint32_t ticks = SDL_GetTicks();
    const bool blink = PlayfieldViewport::isBlinkOn(ticks, 350);
    if (blink) {
        const std::string prompt = "INSERT COIN - PRESS ANY KEY TO PLAY";
        const int xPrompt = std::max(10, (screenW - static_cast<int>(prompt.length()) * 8 * 1) / 2);

        SDL_Rect botBox {xPrompt - 16, screenH - 46, static_cast<int>(prompt.length()) * 8 + 32, 28};
        SDL_SetRenderDrawColor(m_renderer.get(), 15, 23, 42, 220);
        SDL_RenderFillRect(m_renderer.get(), &botBox);
        SDL_SetRenderDrawColor(m_renderer.get(), 74, 222, 128, 255);
        SDL_RenderDrawRect(m_renderer.get(), &botBox);

        BitmapFont::drawText(m_renderer.get(), prompt, xPrompt, screenH - 38, 1, SDL_Color {74, 222, 128, 255});
    }
}

void SdlRenderer::drawInstructionsCard() noexcept
{
    const int screenW = getWidth();
    const int screenH = getHeight();

    const std::string title = "HOW TO PLAY";
    const int xTitle = std::max(10, (screenW - static_cast<int>(title.length()) * 8 * 2) / 2);
    BitmapFont::drawText(m_renderer.get(), title, xTitle, screenH / 2 - 140, 2, SDL_Color {250, 204, 21, 255});

    int y = screenH / 2 - 80;
    for (const auto& r : GamePresenter::getInstructionRules()) {
        const int xH = screenW / 2 - 220;
        BitmapFont::drawText(m_renderer.get(), r.header, xH, y, 1, toSdl(r.color));
        BitmapFont::drawText(m_renderer.get(), r.detail, xH, y + 14, 1, SDL_Color {229, 231, 235, 255});
        y += 36;
    }

    const std::uint32_t ticks = SDL_GetTicks();
    const bool blink = PlayfieldViewport::isBlinkOn(ticks, 350);
    if (blink) {
        const std::string prompt = "INSERT COIN - PRESS ANY KEY TO PLAY";
        const int xPrompt = std::max(10, (screenW - static_cast<int>(prompt.length()) * 8 * 1) / 2);
        BitmapFont::drawText(m_renderer.get(), prompt, xPrompt, screenH / 2 + 130, 1, SDL_Color {74, 222, 128, 255});
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

} // namespace qix::sdl
