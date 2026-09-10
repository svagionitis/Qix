#include "RaylibRenderer.h"
#include "BackgroundArt.h"
#include "GamePresenter.h"
#include "HighScoreTable.h"
#include "PlayfieldViewport.h"
#include <algorithm>
#include <cstdio>

namespace {
[[nodiscard]] constexpr Color toRaylib(qix::PaletteColor c) noexcept
{
    return Color {c.r, c.g, c.b, c.a};
}
} // namespace

namespace qix::raylib {

RaylibRenderer::~RaylibRenderer()
{
    if (m_artTextureInitialized) {
        UnloadTexture(m_artTexture);
        m_artTextureInitialized = false;
    }
    if (m_targetInitialized) {
        UnloadRenderTexture(m_targetTexture);
        m_targetInitialized = false;
    }
    if (m_initialized) {
        CloseWindow();
        m_initialized = false;
    }
}

void RaylibRenderer::setPalette(PaletteId id) noexcept
{
    m_paletteId = id;
}

PaletteId RaylibRenderer::getPalette() const noexcept
{
    return m_paletteId;
}

void RaylibRenderer::cyclePalette() noexcept
{
    m_paletteId = ColorPalette::next(m_paletteId);
}

void RaylibRenderer::setArtEnabled(bool enabled) noexcept
{
    m_artEnabled = enabled;
}

bool RaylibRenderer::isArtEnabled() const noexcept
{
    return m_artEnabled;
}

void RaylibRenderer::toggleArt() noexcept
{
    m_artEnabled = !m_artEnabled;
}

void RaylibRenderer::setArtScene(int scene) noexcept
{
    m_forcedArtScene = scene;
}

void RaylibRenderer::ensureArtTexture(ArtScene scene) noexcept
{
    if (m_artTextureInitialized && m_currentScene == scene) {
        return;
    }

    if (m_artTextureInitialized) {
        UnloadTexture(m_artTexture);
        m_artTextureInitialized = false;
    }

    constexpr int ArtW = 320;
    constexpr int ArtH = 240;
    std::vector<std::uint8_t> buffer;
    BackgroundArt::generateRgbaBuffer(scene, ArtW, ArtH, buffer);

    Image img {};
    img.data = buffer.data();
    img.width = ArtW;
    img.height = ArtH;
    img.mipmaps = 1;
    img.format = PIXELFORMAT_UNCOMPRESSED_R8G8B8A8;

    m_artTexture = LoadTextureFromImage(img);
    m_artTextureInitialized = true;
    m_currentScene = scene;
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

void RaylibRenderer::setCrtEnabled(bool enabled) noexcept
{
    m_crtEnabled = enabled;
}

bool RaylibRenderer::isCrtEnabled() const noexcept
{
    return m_crtEnabled;
}

void RaylibRenderer::toggleCrt() noexcept
{
    m_crtEnabled = !m_crtEnabled;
}

void RaylibRenderer::onSimulationTick(const GameView& view) noexcept
{
    m_interpolator.onTick(view);
}

void RaylibRenderer::resetInterpolation() noexcept
{
    m_interpolator.reset();
}

void RaylibRenderer::render(const GameView& view, std::uint32_t delayMs, float alpha) noexcept
{
    if (!m_initialized) {
        return;
    }

    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    if (m_crtEnabled) {
        if (!m_targetInitialized || m_targetTexture.texture.width != screenW
            || m_targetTexture.texture.height != screenH) {
            if (m_targetInitialized) {
                UnloadRenderTexture(m_targetTexture);
            }
            m_targetTexture = LoadRenderTexture(screenW, screenH);
            m_targetInitialized = true;
        }
        BeginTextureMode(m_targetTexture);
    } else {
        BeginDrawing();
    }

    const auto& theme = ColorPalette::get(m_paletteId);

    const auto activeScene = (m_forcedArtScene >= 0) ? BackgroundArt::fromIndex(m_forcedArtScene)
                                                     : BackgroundArt::getSceneForLevel(view.stats.level);

    if (m_artEnabled) {
        ensureArtTexture(activeScene);
    }

    // 1. Clear background
    ClearBackground(toRaylib(theme.background));

    // 2. Render Top HUD Bar
    drawHud(view.stats, delayMs);

    // 3. Render Playfield
    const float hudHeight = 50.0f;
    const float margin = 20.0f;

    const Rectangle fieldRect {margin, hudHeight, std::max(10.0f, static_cast<float>(screenW) - 2.0f * margin),
        std::max(10.0f, static_cast<float>(screenH) - hudHeight - margin)};

    // Event & continuous particle handling
    for (const auto& evt : view.events) {
        if (evt.type == GameEventType::MarkerDeath) {
            m_particles.emitMarkerExplosion(evt.position);
        } else if (evt.type == GameEventType::TerritoryCapture) {
            m_particles.emitCaptureFlash(evt.capturePerimeter, evt.drawMode);
        }
    }

    const float dt = GetFrameTime();
    if (view.fusePos.has_value()) {
        m_particles.emitFuseSparkles(view.fusePos.value(), view.stixTrail, dt);
    }
    m_particles.update(dt);

    if (view.playfield) {
        drawPlayfield(*view.playfield, fieldRect, view);
        drawQixRibbons(view.qixRibbons, fieldRect);
        drawEntities(view, fieldRect, alpha);
        drawParticles(fieldRect);
    }

    // 4. Overlays
    drawOverlays(view, fieldRect);

    if (m_crtEnabled) {
        EndTextureMode();

        BeginDrawing();
        ClearBackground(toRaylib(theme.crtBackdrop));
        applyCrtFilter(screenW, screenH);
        EndDrawing();
    } else {
        EndDrawing();
    }

    ++m_colorCycle;
}

void RaylibRenderer::applyCrtFilter(int width, int height) noexcept
{
    const auto fW = static_cast<float>(width);
    const auto fH = static_cast<float>(height);

    // Invert Y coordinate because Raylib/OpenGL render textures are flipped vertically
    const Rectangle srcRect {0.0f, 0.0f, fW, -fH};
    const Rectangle destRect {0.0f, 0.0f, fW, fH};
    const Vector2 origin {0.0f, 0.0f};

    // 1. Base scene render
    DrawTexturePro(m_targetTexture.texture, srcRect, destRect, origin, 0.0f, WHITE);

    // 2. Additive phosphor bloom pass
    BeginBlendMode(BLEND_ADDITIVE);
    const float offsets[4][2] = {{-1.5f, 0.0f}, {1.5f, 0.0f}, {0.0f, -1.5f}, {0.0f, 1.5f}};
    for (const auto& off : offsets) {
        const Rectangle bloomDest {off[0], off[1], fW, fH};
        DrawTexturePro(m_targetTexture.texture, srcRect, bloomDest, origin, 0.0f, Color {255, 255, 255, 60});
    }
    const float wideOffsets[4][2] = {{-2.5f, -1.5f}, {2.5f, 1.5f}, {-1.5f, 2.5f}, {1.5f, -2.5f}};
    for (const auto& off : wideOffsets) {
        const Rectangle wideDest {off[0], off[1], fW, fH};
        DrawTexturePro(m_targetTexture.texture, srcRect, wideDest, origin, 0.0f, Color {255, 255, 255, 25});
    }
    EndBlendMode();

    // 3. Horizontal raster scanlines
    for (int y = 0; y < height; y += 2) {
        DrawRectangle(0, y, width, 1, Color {0, 0, 0, 80});
    }

    // 4. CRT Vignette & Curved Bezel Shadow
    for (int b = 0; b < 10; ++b) {
        const auto alpha = static_cast<unsigned char>((10 - b) * 14);
        DrawRectangle(0, b, width, 1, Color {0, 0, 0, alpha});
        DrawRectangle(0, height - 1 - b, width, 1, Color {0, 0, 0, alpha});
        DrawRectangle(b, 0, 1, height, Color {0, 0, 0, alpha});
        DrawRectangle(width - 1 - b, 0, 1, height, Color {0, 0, 0, alpha});
    }
    for (int c = 0; c < 14; ++c) {
        const auto alpha = static_cast<unsigned char>((14 - c) * 12);
        DrawLine(0, c, 14 - c, 0, Color {0, 0, 0, alpha});
        DrawLine(width - 1 - (14 - c), 0, width - 1, c, Color {0, 0, 0, alpha});
        DrawLine(0, height - 1 - c, 14 - c, height - 1, Color {0, 0, 0, alpha});
        DrawLine(width - 1 - (14 - c), height - 1, width - 1, height - 1 - c, Color {0, 0, 0, alpha});
    }
}

void RaylibRenderer::drawHud(const GameStats& stats, std::uint32_t delayMs) noexcept
{
    const int screenW = GetScreenWidth();
    const auto& theme = ColorPalette::get(m_paletteId);

    // Top status bar background
    DrawRectangle(0, 0, screenW, 45, toRaylib(theme.hudBg));
    DrawLine(0, 45, screenW, 45, toRaylib(theme.hudBorder));

    const Color labelColor = toRaylib(theme.textLabel);
    const Color valueColor = toRaylib(theme.textValue);
    const Color accentColor = toRaylib(theme.textAccent);
    const Color greenColor = toRaylib(theme.progressBarTarget);
    const Color redColor = toRaylib(theme.markerDiamond);

    const int fontSize = 16;
    const int textY = 15;

    const auto hud = GamePresenter::formatHud(stats, delayMs);

    // 1. SCORE & HIGH
    DrawText("SCORE:", 15, textY, fontSize, labelColor);
    DrawText(hud.scoreStr.c_str(), 72, textY, fontSize, valueColor);
    DrawText("HIGH:", 135, textY, fontSize, labelColor);
    DrawText(hud.hiScoreStr.c_str(), 180, textY, fontSize, valueColor);

    // 2. CLAIMED % & Progress Bar
    DrawText("CLAIM:", 255, textY, fontSize, labelColor);
    DrawText(hud.claimStr.c_str(), 310, textY, fontSize, hud.targetReached ? greenColor : accentColor);

    const int barX = 405;
    const int barY = 16;
    const int barW = 75;
    const int barH = 14;
    DrawRectangle(barX, barY, barW, barH, toRaylib(theme.progressBarBg));
    const int fillW = std::min(barW, (barW * hud.fillPercent) / 100);
    DrawRectangle(barX, barY, fillW, barH, hud.targetReached ? greenColor : toRaylib(theme.progressBarFill));

    // 3. LIVES
    DrawText("LIVES:", 495, textY, fontSize, labelColor);
    for (int i = 0; i < stats.lives; ++i) {
        DrawPoly(Vector2 {static_cast<float>(550 + i * 16), 23.0f}, 4, 6.0f, 45.0f, redColor);
    }

    // 4. TIME
    const Color timerColor = (hud.timeUrgency == HudUrgency::Critical)
        ? redColor
        : ((hud.timeUrgency == HudUrgency::Warning) ? valueColor : greenColor);
    DrawText("TIME:", screenW - 365, textY, fontSize, labelColor);
    DrawText(hud.timeStr.c_str(), screenW - 320, textY, fontSize, timerColor);

    // 5. MULTIPLIER / SPEED / LEVEL
    if (!hud.multiplierStr.empty()) {
        DrawText("MULT:", screenW - 265, textY, fontSize, labelColor);
        DrawText(hud.multiplierStr.c_str(), screenW - 220, textY, fontSize, valueColor);

        DrawText("SPD:", screenW - 165, textY, fontSize, labelColor);
        DrawText(hud.speedStr.c_str(), screenW - 125, textY, fontSize, accentColor);

        DrawText("LVL:", screenW - 65, textY, fontSize, labelColor);
        DrawText(hud.levelStr.c_str(), screenW - 25, textY, fontSize, valueColor);
    } else {
        DrawText("SPEED:", screenW - 200, textY, fontSize, labelColor);
        DrawText(hud.speedStr.c_str(), screenW - 145, textY, fontSize, accentColor);

        DrawText("LEVEL:", screenW - 85, textY, fontSize, labelColor);
        DrawText(hud.levelStr.c_str(), screenW - 30, textY, fontSize, valueColor);
    }
}

void RaylibRenderer::drawPlayfield(
    const Playfield& playfield, const Rectangle& fieldRect, const GameView& view) noexcept
{
    (void)view;
    const auto gridW = playfield.getWidth();
    const auto gridH = playfield.getHeight();
    if (gridW <= 0 || gridH <= 0) {
        return;
    }

    const auto& theme = ColorPalette::get(m_paletteId);
    const PlayfieldViewport vp {fieldRect.x, fieldRect.y, fieldRect.width, fieldRect.height, gridW, gridH};
    const float cellW = vp.cellWidth();
    const float cellH = vp.cellHeight();
    const int texW = m_artTextureInitialized ? m_artTexture.width : 1;
    const int texH = m_artTextureInitialized ? m_artTexture.height : 1;

    for (std::int32_t y {0}; y < gridH; ++y) {
        for (std::int32_t x {0}; x < gridW; ++x) {
            const auto state = playfield.getCell(x, y);
            if (state == CellState::Empty) {
                continue;
            }

            const auto vr = vp.cellToScreen(x, y);
            const Rectangle cellRect {vr.x, vr.y, vr.width, vr.height};

            if (state == CellState::ClaimedSlow || state == CellState::ClaimedFast) {
                if (m_artEnabled && m_artTextureInitialized) {
                    const auto sr = vp.cellToTextureSrc(x, y, texW, texH);
                    const Rectangle srcRect {static_cast<float>(sr.x), static_cast<float>(sr.y),
                        static_cast<float>(sr.width), static_cast<float>(sr.height)};
                    DrawTexturePro(m_artTexture, srcRect, cellRect, {0.0f, 0.0f}, 0.0f, WHITE);
                }
                DrawRectangleRec(
                    cellRect, toRaylib(state == CellState::ClaimedSlow ? theme.claimedSlow : theme.claimedFast));
            } else if (state == CellState::Border) {
                // Fill the half of the border cell facing any claimed neighbor so claimed territory
                // meets the thin vector line seamlessly without gaps
                const float cx = fieldRect.x + (static_cast<float>(x) + 0.5f) * cellW;
                const float cy = fieldRect.y + (static_cast<float>(y) + 0.5f) * cellH;

                auto fillHalf = [&](int dx, int dy, CellState neighborState) {
                    const auto col
                        = toRaylib(neighborState == CellState::ClaimedSlow ? theme.claimedSlow : theme.claimedFast);
                    if (dx > 0) {
                        DrawRectangleRec(Rectangle {cx, vr.y, cellW * 0.5f + 0.5f, vr.height}, col);
                    } else if (dx < 0) {
                        DrawRectangleRec(Rectangle {vr.x, vr.y, cellW * 0.5f + 0.5f, vr.height}, col);
                    } else if (dy > 0) {
                        DrawRectangleRec(Rectangle {vr.x, cy, vr.width, cellH * 0.5f + 0.5f}, col);
                    } else if (dy < 0) {
                        DrawRectangleRec(Rectangle {vr.x, vr.y, vr.width, cellH * 0.5f + 0.5f}, col);
                    }
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

    // Render slender 1.5px vector lines connecting adjacent border cells
    const auto borderCol = toRaylib(theme.playfieldBorder);
    for (std::int32_t y {0}; y < gridH; ++y) {
        for (std::int32_t x {0}; x < gridW; ++x) {
            if (playfield.getCell(x, y) != CellState::Border) {
                continue;
            }
            const Vector2 c1 {fieldRect.x + (static_cast<float>(x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(y) + 0.5f) * cellH};

            if (x + 1 < gridW && playfield.getCell(x + 1, y) == CellState::Border) {
                const Vector2 c2 {fieldRect.x + (static_cast<float>(x + 1) + 0.5f) * cellW, c1.y};
                DrawLineEx(c1, c2, 1.5f, borderCol);
            }
            if (y + 1 < gridH && playfield.getCell(x, y + 1) == CellState::Border) {
                const Vector2 c2 {c1.x, fieldRect.y + (static_cast<float>(y + 1) + 0.5f) * cellH};
                DrawLineEx(c1, c2, 1.5f, borderCol);
            }
        }
    }
}

void RaylibRenderer::drawQixRibbons(
    const std::vector<std::deque<LineSegment>>& ribbons, const Rectangle& fieldRect) noexcept
{
    const PlayfieldViewport vp {fieldRect.x, fieldRect.y, fieldRect.width, fieldRect.height, 80, 60};
    const float cellW = vp.cellWidth();
    const float cellH = vp.cellHeight();
    const auto& theme = ColorPalette::get(m_paletteId);

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

            const auto col = ColorPalette::computeRibbonColor(theme, m_colorCycle, segIndex, totalSegs);
            DrawLineEx(start, end, (segIndex == 0) ? 3.0f : 2.0f, toRaylib(col));

            ++segIndex;
        }
    }

    EndBlendMode();
}

void RaylibRenderer::drawEntities(const GameView& view, const Rectangle& fieldRect, float alpha) noexcept
{
    const float cellW = fieldRect.width / 80.0f;
    const float cellH = fieldRect.height / 60.0f;
    const auto& theme = ColorPalette::get(m_paletteId);

    // 1. Active Stix Trail
    if (view.stixTrail.size() >= 2) {
        for (std::size_t i = 1; i < view.stixTrail.size(); ++i) {
            const Vector2 p1 {fieldRect.x + (static_cast<float>(view.stixTrail[i - 1].x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(view.stixTrail[i - 1].y) + 0.5f) * cellH};
            const Vector2 p2 {fieldRect.x + (static_cast<float>(view.stixTrail[i].x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(view.stixTrail[i].y) + 0.5f) * cellH};
            DrawLineEx(p1, p2, 1.5f, toRaylib(theme.activeStix));
        }
    }

    // 2. Sparx (Linearly interpolated screen positions)
    if (!view.sparxList.empty()) {
        for (std::size_t i = 0; i < view.sparxList.size(); ++i) {
            const auto& sp = view.sparxList[i];
            const auto [sx, sy] = m_interpolator.interpolateSparx(i, sp.position, alpha);
            const Vector2 center {fieldRect.x + (sx + 0.5f) * cellW, fieldRect.y + (sy + 0.5f) * cellH};
            if (sp.isSuper) {
                DrawPoly(center, 4, 8.0f, 45.0f, toRaylib(theme.superSparx));
                DrawPoly(center, 4, 4.0f, 45.0f, WHITE);
            } else {
                DrawPoly(center, 4, 6.0f, 45.0f, toRaylib(theme.sparx));
            }
        }
    } else {
        for (std::size_t i = 0; i < view.sparxPositions.size(); ++i) {
            const auto& sp = view.sparxPositions[i];
            const auto [sx, sy] = m_interpolator.interpolateSparx(i, sp, alpha);
            const Vector2 center {fieldRect.x + (sx + 0.5f) * cellW, fieldRect.y + (sy + 0.5f) * cellH};
            DrawPoly(center, 4, 6.0f, 45.0f, toRaylib(theme.sparx));
        }
    }

    // 3. Fuse
    if (view.fusePos.has_value()) {
        const auto fp = view.fusePos.value();
        const Vector2 center {fieldRect.x + (static_cast<float>(fp.x) + 0.5f) * cellW,
            fieldRect.y + (static_cast<float>(fp.y) + 0.5f) * cellH};
        DrawCircleV(center, 5.0f, toRaylib(theme.textValue));
        DrawCircleV(center, 3.0f, toRaylib(theme.fuse));
    }

    // 4. Player Marker (Linearly interpolated screen position)
    const auto [mx, my] = m_interpolator.interpolateMarker(view.markerPos, alpha);
    const Vector2 markerPos {fieldRect.x + (mx + 0.5f) * cellW, fieldRect.y + (my + 0.5f) * cellH};
    const Color markerColor = (view.drawMode != DrawMode::None) ? toRaylib(theme.textValue) : toRaylib(theme.marker);
    DrawPoly(markerPos, 4, 7.0f, 45.0f, markerColor);
}

void RaylibRenderer::drawParticles(const Rectangle& fieldRect) noexcept
{
    if (m_particles.empty()) {
        return;
    }

    const float cellW = fieldRect.width / 80.0f;
    const float cellH = fieldRect.height / 60.0f;
    const auto* parts = m_particles.data();
    const std::size_t count = m_particles.size();

    for (std::size_t i = 0; i < count; ++i) {
        const auto& p = parts[i];
        const Vector2 pos {fieldRect.x + p.x * cellW, fieldRect.y + p.y * cellH};
        const Color col = toRaylib(p.currentColor());
        const float pixelSize = std::max(1.5f, p.size * cellW);

        switch (p.type) {
        case ParticleType::Spark: {
            const float speed = std::hypot(p.vx, p.vy);
            const float trailLen = std::clamp(speed * 0.04f * cellW, 2.0f, 12.0f);
            const float normVx = (speed > 0.001f) ? (p.vx / speed) : 0.0f;
            const float normVy = (speed > 0.001f) ? (p.vy / speed) : 0.0f;
            const Vector2 p2 {pos.x - normVx * trailLen, pos.y - normVy * trailLen};
            DrawLineEx(pos, p2, std::max(1.5f, pixelSize * 0.6f), col);
            break;
        }
        case ParticleType::GlowShard:
        case ParticleType::DebrisDiamond: {
            const float rotDeg = p.rotation * (180.0f / 3.14159265f);
            DrawPoly(pos, 4, pixelSize, rotDeg, col);
            break;
        }
        case ParticleType::DebrisSquare: {
            const Rectangle rect {pos.x, pos.y, pixelSize * 1.5f, pixelSize * 1.5f};
            const Vector2 origin {rect.width * 0.5f, rect.height * 0.5f};
            const float rotDeg = p.rotation * (180.0f / 3.14159265f);
            DrawRectanglePro(rect, origin, rotDeg, col);
            break;
        }
        case ParticleType::DebrisLine: {
            const float halfLen = pixelSize * 1.6f;
            const float cosR = std::cos(p.rotation);
            const float sinR = std::sin(p.rotation);
            const Vector2 p1 {pos.x - cosR * halfLen, pos.y - sinR * halfLen};
            const Vector2 p2 {pos.x + cosR * halfLen, pos.y + sinR * halfLen};
            DrawLineEx(p1, p2, 2.0f, col);
            break;
        }
        }
    }
}

void RaylibRenderer::drawOverlays(const GameView& view, const Rectangle& fieldRect) noexcept
{
    if (view.state == GameState::Playing || view.state == GameState::Ready) {
        if (view.isPaused) {
            drawPauseOverlay(view.stats);
        }
        return;
    }

    if (view.state == GameState::Attract) {
        if (view.attractStage == AttractStage::GameplayDemo) {
            drawDemoBanners();
            return;
        }
    }

    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    if (view.state == GameState::LevelComplete && m_artEnabled && m_artTextureInitialized) {
        // Grand victory curtain reveal: full background art unmasked!
        const Rectangle fullSrc {
            0.0f, 0.0f, static_cast<float>(m_artTexture.width), static_cast<float>(m_artTexture.height)};
        DrawTexturePro(m_artTexture, fullSrc, fieldRect, {0.0f, 0.0f}, 0.0f, Color {255, 255, 255, 230});
        DrawRectangleLinesEx(fieldRect, 3.0f, Color {250, 204, 21, 255});
        // Soft backdrop behind victory text
        DrawRectangle(0, screenH / 2 - 95, screenW, 190, Color {0, 0, 0, 190});
    } else {
        // Semi-transparent blackout
        DrawRectangle(0, 0, screenW, screenH, Color {0, 0, 0, 200});
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
        if (m_artEnabled && m_artTextureInitialized) {
            const std::string sceneBanner = std::string("ART UNMASKED: ") + BackgroundArt::getSceneName(m_currentScene);
            const int fontS = 18;
            const int ws = MeasureText(sceneBanner.c_str(), fontS);
            DrawText(sceneBanner.c_str(), (screenW - ws) / 2, screenH / 2 - 80, fontS, Color {255, 215, 0, 255});
        }

        const auto pres = GamePresenter::formatVictory(view.stats);
        const int fontTitle = 28;
        const int wt = MeasureText(pres.title.c_str(), fontTitle);
        const int titleY = pres.isTrap ? (screenH / 2 - 55) : (screenH / 2 - 35);
        DrawText(pres.title.c_str(), (screenW - wt) / 2, titleY, fontTitle, toRaylib(pres.titleColor));

        if (pres.hasDetail) {
            const int fontD = 20;
            const int wd = MeasureText(pres.detail.c_str(), fontD);
            DrawText(pres.detail.c_str(), (screenW - wd) / 2, screenH / 2 - 25, fontD, Color {56, 189, 248, 255});
        }

        if (pres.hasBonus) {
            const int fontB = 20;
            const int wb = MeasureText(pres.bonus.c_str(), fontB);
            const int yb = pres.isTrap ? (screenH / 2 + 2) : (screenH / 2);
            DrawText(pres.bonus.c_str(), (screenW - wb) / 2, yb, fontB, Color {250, 204, 21, 255});
        }

        const int fontP = 18;
        const int wp = MeasureText(pres.prompt.c_str(), fontP);
        const int yp = pres.isTrap
            ? (screenH / 2 + 32)
            : ((!view.stats.splitBonus && pres.hasBonus) ? (screenH / 2 + 28) : (screenH / 2 + 15));
        DrawText(pres.prompt.c_str(), (screenW - wp) / 2, yp, fontP, Color {243, 244, 246, 255});
    } else if (view.state == GameState::NameEntry) {
        drawNameEntry(view.nameEntry, view.stats);
    } else if (view.state == GameState::HallOfFame) {
        drawHallOfFame(view.highScoreTable, false, false);
    } else if (view.state == GameState::GameOver) {
        drawHallOfFame(view.highScoreTable, true, false);
    }
}

void RaylibRenderer::drawNameEntry(const NameEntryState& entry, const GameStats& stats) noexcept
{
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    const char* title = "ARCADE HALL OF FAME";
    const int fontTitle = 30;
    const int wTitle = MeasureText(title, fontTitle);
    DrawText(title, (screenW - wTitle) / 2, screenH / 2 - 140, fontTitle, Color {250, 204, 21, 255});

    const auto rankStr = GamePresenter::formatRecordBanner(entry.rank, stats.score);
    const int fontScore = 18;
    const int wScore = MeasureText(rankStr.c_str(), fontScore);
    DrawText(rankStr.c_str(), (screenW - wScore) / 2, screenH / 2 - 95, fontScore, Color {226, 232, 240, 255});

    const int boxW = 54;
    const int boxH = 64;
    const int gap = 16;
    const int totalW = 3 * boxW + 2 * gap;
    const int startX = (screenW - totalW) / 2;
    const int boxY = screenH / 2 - 40;

    for (std::uint8_t i {0}; i < 3; ++i) {
        const int bx = startX + static_cast<int>(i) * (boxW + gap);
        const bool isActive = (entry.cursorIndex == i);

        DrawRectangle(bx, boxY, boxW, boxH, Color {15, 23, 42, 255});
        DrawRectangleLinesEx(Rectangle {static_cast<float>(bx), static_cast<float>(boxY), static_cast<float>(boxW),
                                 static_cast<float>(boxH)},
            2.0f, isActive ? Color {250, 204, 21, 255} : Color {71, 85, 105, 255});

        if (isActive) {
            DrawText("^", bx + boxW / 2 - 5, boxY - 18, 18, Color {250, 204, 21, 255});
            DrawText("v", bx + boxW / 2 - 5, boxY + boxH + 2, 18, Color {250, 204, 21, 255});
        }

        const char letterStr[2] = {entry.initials[i], '\0'};
        const int fontChar = 32;
        const int wChar = MeasureText(letterStr, fontChar);
        DrawText(letterStr, bx + (boxW - wChar) / 2, boxY + 14, fontChar, Color {248, 250, 252, 255});
    }

    const char* navHelp = "[UP/DOWN] Letter   [LEFT/RIGHT] Slot   [ENTER/SPACE] Confirm";
    const int fontHelp = 14;
    const int wHelp = MeasureText(navHelp, fontHelp);
    DrawText(navHelp, (screenW - wHelp) / 2, screenH / 2 + 75, fontHelp, Color {148, 163, 184, 255});
}

void RaylibRenderer::drawHallOfFame(const HighScoreTable* table, bool isGameOver, bool isAttract) noexcept
{
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    int curY = screenH / 2 - 160;

    if (isGameOver) {
        const char* goText = "GAME OVER";
        const int fontGo = 26;
        const int wGo = MeasureText(goText, fontGo);
        DrawText(goText, (screenW - wGo) / 2, curY, fontGo, Color {248, 113, 113, 255});
        curY += 35;
    } else if (isAttract) {
        const char* attText = "TAITO 1981 - QIX ARCADE";
        const int fontAtt = 24;
        const int wAtt = MeasureText(attText, fontAtt);
        DrawText(attText, (screenW - wAtt) / 2, curY, fontAtt, Color {59, 130, 246, 255});
        curY += 32;
    }

    const char* title = "ARCADE HALL OF FAME";
    const int fontTitle = 22;
    const int wTitle = MeasureText(title, fontTitle);
    DrawText(title, (screenW - wTitle) / 2, curY, fontTitle, Color {250, 204, 21, 255});
    curY += 32;

    const char* header = "RANK     NAME       SCORE      LVL    MODE";
    const int fontRow = 15;
    const int wHeader = MeasureText(header, fontRow);
    DrawText(header, (screenW - wHeader) / 2, curY, fontRow, Color {99, 179, 237, 255});
    curY += 22;

    DrawLine((screenW - wHeader) / 2, curY, (screenW + wHeader) / 2, curY, Color {51, 65, 85, 255});
    curY += 8;

    if (table != nullptr) {
        const auto rows = GamePresenter::formatHallOfFame(table, 7);
        for (const auto& r : rows) {
            const int wRow = MeasureText(r.formattedRow.c_str(), fontRow);
            DrawText(r.formattedRow.c_str(), (screenW - wRow) / 2, curY, fontRow, toRaylib(r.medalColor));
            curY += 20;
        }
    }

    curY += 15;
    const bool blink = (static_cast<int>(GetTime() * 3.0) % 2 == 0);
    const auto prompt = GamePresenter::formatHofPrompt(isAttract, blink);
    if (!prompt.empty()) {
        const int fontPrompt = 16;
        const int wPrompt = MeasureText(prompt.c_str(), fontPrompt);
        DrawText(prompt.c_str(), (screenW - wPrompt) / 2, curY, fontPrompt,
            isAttract ? Color {74, 222, 128, 255} : Color {243, 244, 246, 255});
    }
}

void RaylibRenderer::drawDemoBanners() noexcept
{
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    // Top banner card
    DrawRectangle(screenW / 2 - 170, 16, 340, 36, Color {15, 23, 42, 220});
    DrawRectangleLinesEx(
        Rectangle {static_cast<float>(screenW / 2 - 170), 16.0f, 340.0f, 36.0f}, 2.0f, Color {250, 204, 21, 255});

    const char* demoText = "*** GAMEPLAY DEMO ***";
    const int fontDemo = 20;
    const int wDemo = MeasureText(demoText, fontDemo);
    DrawText(demoText, (screenW - wDemo) / 2, 24, fontDemo, Color {250, 204, 21, 255});

    // Bottom blinking prompt
    const bool blink = (static_cast<int>(GetTime() * 3.0) % 2 == 0);
    if (blink) {
        DrawRectangle(screenW / 2 - 220, screenH - 54, 440, 34, Color {15, 23, 42, 220});
        DrawRectangleLinesEx(
            Rectangle {static_cast<float>(screenW / 2 - 220), static_cast<float>(screenH - 54), 440.0f, 34.0f}, 1.5f,
            Color {74, 222, 128, 255});

        const char* prompt = "INSERT COIN - PRESS ANY KEY TO PLAY";
        const int fontPrompt = 18;
        const int wPrompt = MeasureText(prompt, fontPrompt);
        DrawText(prompt, (screenW - wPrompt) / 2, screenH - 46, fontPrompt, Color {74, 222, 128, 255});
    }
}

void RaylibRenderer::drawInstructionsCard() noexcept
{
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    const char* title = "HOW TO PLAY";
    const int fontTitle = 30;
    const int wTitle = MeasureText(title, fontTitle);
    DrawText(title, (screenW - wTitle) / 2, screenH / 2 - 150, fontTitle, Color {250, 204, 21, 255});

    int y = screenH / 2 - 100;
    for (const auto& rule : GamePresenter::getInstructionRules()) {
        DrawText(rule.header, screenW / 2 - 270, y, 16, toRaylib(rule.color));
        DrawText(rule.detail, screenW / 2 - 270, y + 20, 15, Color {229, 231, 235, 255});
        y += 48;
    }

    const bool blink = (static_cast<int>(GetTime() * 3.0) % 2 == 0);
    if (blink) {
        const char* prompt = "INSERT COIN - PRESS ANY KEY TO PLAY";
        const int fontP = 18;
        const int wp = MeasureText(prompt, fontP);
        DrawText(prompt, (screenW - wp) / 2, screenH / 2 + 155, fontP, Color {74, 222, 128, 255});
    }
}

void RaylibRenderer::drawPauseOverlay(const GameStats& stats) noexcept
{
    const auto pres = GamePresenter::formatPauseOverlay(stats);
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    // 1. Semi-transparent backdrop
    DrawRectangle(0, 0, screenW, screenH, Color {10, 15, 26, 215});

    // 2. Center modal card
    const int cardW = std::min(640, screenW - 30);
    const int cardH = std::min(460, screenH - 40);
    const int cardX = (screenW - cardW) / 2;
    const int cardY = (screenH - cardH) / 2;

    const Rectangle cardRect {
        static_cast<float>(cardX), static_cast<float>(cardY), static_cast<float>(cardW), static_cast<float>(cardH)};
    DrawRectangleRounded(cardRect, 0.05f, 8, Color {15, 23, 42, 235});
    DrawRectangleLinesEx(cardRect, 2.0f, Color {59, 130, 246, 220});

    // Header Title
    const int fontTitle = 24;
    const int wTitle = MeasureText(pres.title.c_str(), fontTitle);
    DrawText(pres.title.c_str(), cardX + (cardW - wTitle) / 2, cardY + 16, fontTitle, Color {250, 204, 21, 255});

    // Level and Target Info
    const int fontSub = 16;
    const int wLevel = MeasureText(pres.levelInfo.c_str(), fontSub);
    DrawText(pres.levelInfo.c_str(), cardX + (cardW - wLevel) / 2, cardY + 48, fontSub, Color {96, 165, 250, 255});

    const int wTarget = MeasureText(pres.targetInfo.c_str(), fontSub);
    DrawText(pres.targetInfo.c_str(), cardX + (cardW - wTarget) / 2, cardY + 70, fontSub, Color {52, 211, 153, 255});

    // Horizontal divider
    DrawLine(cardX + 24, cardY + 96, cardX + cardW - 24, cardY + 96, Color {71, 85, 105, 180});

    // Controls Column
    DrawText("CONTROLS", cardX + 24, cardY + 106, 16, Color {245, 158, 11, 255});
    int ctrlY = cardY + 130;
    for (const auto& c : pres.controls) {
        DrawText(c.action, cardX + 24, ctrlY, 14, Color {229, 231, 235, 255});
        DrawText(c.keys, cardX + 175, ctrlY, 14, Color {147, 197, 253, 255});
        ctrlY += 22;
    }

    // Scoring Column
    const int scoreColX = cardX + cardW / 2 + 10;
    DrawText("SCORING", scoreColX, cardY + 106, 16, Color {245, 158, 11, 255});
    int scoreY = cardY + 130;
    for (const auto& s : pres.scoring) {
        DrawText(s.label, scoreColX, scoreY, 14, Color {253, 224, 71, 255});
        DrawText(s.points, scoreColX, scoreY + 17, 12, Color {209, 213, 219, 255});
        scoreY += 36;
    }

    // Bottom divider & Resume Prompt
    DrawLine(cardX + 24, cardY + cardH - 45, cardX + cardW - 24, cardY + cardH - 45, Color {71, 85, 105, 180});

    const bool blink = (static_cast<int>(GetTime() * 3.0) % 2 == 0);
    if (blink) {
        const int fontP = 16;
        const int wp = MeasureText(pres.resumePrompt.c_str(), fontP);
        DrawText(
            pres.resumePrompt.c_str(), cardX + (cardW - wp) / 2, cardY + cardH - 34, fontP, Color {74, 222, 128, 255});
    }
}

} // namespace qix::raylib
