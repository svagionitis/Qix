#include "RaylibRenderer.h"
#include "HighScoreTable.h"
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

void RaylibRenderer::render(const GameView& view, std::uint32_t delayMs) noexcept
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

    // 1. Clear background
    ClearBackground(toRaylib(theme.background));

    // 2. Render Top HUD Bar
    drawHud(view.stats, delayMs);

    // 3. Render Playfield
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
    drawOverlays(view);

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
    const Color purpleColor = valueColor;
    const Color speedColor = accentColor;

    const int fontSize = 16;
    const int textY = 15;

    // 1. SCORE
    DrawText("SCORE:", 15, textY, fontSize, labelColor);
    DrawText(std::to_string(stats.score).c_str(), 72, textY, fontSize, valueColor);

    // HIGH
    DrawText("HIGH:", 135, textY, fontSize, labelColor);
    DrawText(std::to_string(stats.highScore).c_str(), 180, textY, fontSize, valueColor);

    // 2. CLAIMED %
    DrawText("CLAIM:", 255, textY, fontSize, labelColor);
    const std::string claimStr
        = std::to_string(stats.claimedPercent) + "% / " + std::to_string(stats.targetPercent) + "%";
    DrawText(
        claimStr.c_str(), 310, textY, fontSize, stats.claimedPercent >= stats.targetPercent ? greenColor : accentColor);

    // 3. Progress Bar
    const int barX = 405;
    const int barY = 16;
    const int barW = 75;
    const int barH = 14;
    DrawRectangle(barX, barY, barW, barH, toRaylib(theme.progressBarBg));
    const int fillW = std::min(barW, (barW * static_cast<int>(stats.claimedPercent)) / 100);
    DrawRectangle(barX, barY, fillW, barH,
        stats.claimedPercent >= stats.targetPercent ? greenColor : toRaylib(theme.progressBarFill));

    // 4. LIVES
    DrawText("LIVES:", 495, textY, fontSize, labelColor);
    for (int i = 0; i < stats.lives; ++i) {
        DrawPoly(Vector2 {static_cast<float>(550 + i * 16), 23.0f}, 4, 6.0f, 45.0f, redColor);
    }

    // 5. TIME
    const auto secondsRemaining = (stats.timeRemainingMs + 999U) / 1000U;
    const Color timerColor
        = (stats.timeUp || secondsRemaining <= 10U) ? redColor : ((secondsRemaining <= 20U) ? valueColor : greenColor);
    DrawText("TIME:", screenW - 365, textY, fontSize, labelColor);
    const std::string timeStr = std::to_string(secondsRemaining) + "s";
    DrawText(timeStr.c_str(), screenW - 320, textY, fontSize, timerColor);

    // 6. MULTIPLIER / SPEED / LEVEL
    if (stats.multiplier > 1) {
        DrawText("MULT:", screenW - 265, textY, fontSize, labelColor);
        const std::string multStr = std::to_string(stats.multiplier) + "X";
        DrawText(multStr.c_str(), screenW - 220, textY, fontSize, valueColor);

        DrawText("SPD:", screenW - 165, textY, fontSize, labelColor);
        const std::string speedStr = std::to_string(delayMs) + "ms";
        DrawText(speedStr.c_str(), screenW - 125, textY, fontSize, speedColor);

        DrawText("LVL:", screenW - 65, textY, fontSize, labelColor);
        DrawText(std::to_string(stats.level).c_str(), screenW - 25, textY, fontSize, purpleColor);
    } else {
        DrawText("SPEED:", screenW - 200, textY, fontSize, labelColor);
        const std::string speedStr = std::to_string(delayMs) + "ms";
        DrawText(speedStr.c_str(), screenW - 145, textY, fontSize, speedColor);

        DrawText("LEVEL:", screenW - 85, textY, fontSize, labelColor);
        DrawText(std::to_string(stats.level).c_str(), screenW - 30, textY, fontSize, purpleColor);
    }
}

void RaylibRenderer::drawPlayfield(const Playfield& playfield, const Rectangle& fieldRect) noexcept
{
    const auto gridW = playfield.getWidth();
    const auto gridH = playfield.getHeight();
    if (gridW <= 0 || gridH <= 0) {
        return;
    }

    const auto& theme = ColorPalette::get(m_paletteId);
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
                DrawRectangleRec(cellRect, toRaylib(theme.playfieldBorder));
            } else if (state == CellState::ClaimedSlow) {
                DrawRectangleRec(cellRect, toRaylib(theme.claimedSlow));
            } else if (state == CellState::ClaimedFast) {
                DrawRectangleRec(cellRect, toRaylib(theme.claimedFast));
            } else if (state == CellState::ActiveStix) {
                DrawRectangleRec(cellRect, toRaylib(theme.activeStix));
            }
        }
    }
}

void RaylibRenderer::drawQixRibbons(
    const std::vector<std::deque<LineSegment>>& ribbons, const Rectangle& fieldRect) noexcept
{
    const float cellW = fieldRect.width / 80.0f;
    const float cellH = fieldRect.height / 60.0f;
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

            const auto alpha = static_cast<unsigned char>(
                255 - static_cast<int>((segIndex * 180) / std::max<std::size_t>(1, totalSegs)));

            Color lineColor;
            if (theme.ribbonMode == RibbonColorMode::NeonGradient) {
                const float hue
                    = static_cast<float>(static_cast<int>((m_colorCycle * 4 + segIndex * 15) % 120 + 280) % 360);
                lineColor = ColorFromHSV(hue, 0.9f, 1.0f);
            } else if (theme.ribbonMode == RibbonColorMode::MonochromeAmber) {
                const auto val
                    = static_cast<float>(255 - (segIndex * 140) / std::max<std::size_t>(1, totalSegs)) / 255.0f;
                lineColor = ColorFromHSV(38.0f, 0.95f, val);
            } else if (theme.ribbonMode == RibbonColorMode::MonochromeGreen) {
                const auto val
                    = static_cast<float>(255 - (segIndex * 140) / std::max<std::size_t>(1, totalSegs)) / 255.0f;
                lineColor = ColorFromHSV(142.0f, 0.95f, val);
            } else {
                const float hue = static_cast<float>(
                    (m_colorCycle * 5 + segIndex * (360 / std::max<std::size_t>(1, totalSegs))) % 360);
                lineColor = ColorFromHSV(hue, 0.85f, 1.0f);
            }
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
    const auto& theme = ColorPalette::get(m_paletteId);

    // 1. Active Stix Trail
    if (view.stixTrail.size() >= 2) {
        for (std::size_t i = 1; i < view.stixTrail.size(); ++i) {
            const Vector2 p1 {fieldRect.x + (static_cast<float>(view.stixTrail[i - 1].x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(view.stixTrail[i - 1].y) + 0.5f) * cellH};
            const Vector2 p2 {fieldRect.x + (static_cast<float>(view.stixTrail[i].x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(view.stixTrail[i].y) + 0.5f) * cellH};
            DrawLineEx(p1, p2, 2.5f, toRaylib(theme.activeStix));
        }
    }

    // 2. Sparx
    if (!view.sparxList.empty()) {
        for (const auto& sp : view.sparxList) {
            const Vector2 center {fieldRect.x + (static_cast<float>(sp.position.x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(sp.position.y) + 0.5f) * cellH};
            if (sp.isSuper) {
                DrawPoly(center, 4, 8.0f, 45.0f, toRaylib(theme.superSparx));
                DrawPoly(center, 4, 4.0f, 45.0f, WHITE);
            } else {
                DrawPoly(center, 4, 6.0f, 45.0f, toRaylib(theme.sparx));
            }
        }
    } else {
        for (const auto& sp : view.sparxPositions) {
            const Vector2 center {fieldRect.x + (static_cast<float>(sp.x) + 0.5f) * cellW,
                fieldRect.y + (static_cast<float>(sp.y) + 0.5f) * cellH};
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

    // 4. Player Marker
    const Vector2 markerPos {fieldRect.x + (static_cast<float>(view.markerPos.x) + 0.5f) * cellW,
        fieldRect.y + (static_cast<float>(view.markerPos.y) + 0.5f) * cellH};
    const Color markerColor = (view.drawMode != DrawMode::None) ? toRaylib(theme.textValue) : toRaylib(theme.marker);
    DrawPoly(markerPos, 4, 7.0f, 45.0f, markerColor);
}

void RaylibRenderer::drawOverlays(const GameView& view) noexcept
{
    if (view.state == GameState::Playing || view.state == GameState::Ready) {
        return;
    }

    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    // Semi-transparent blackout
    DrawRectangle(0, 0, screenW, screenH, Color {0, 0, 0, 200});

    if (view.state == GameState::LevelComplete) {
        const char* text1 = view.stats.splitBonus ? "QIX SPLIT BONUS!" : "LEVEL COMPLETE!";
        const int font1 = 28;
        const int w1 = MeasureText(text1, font1);
        DrawText(text1, (screenW - w1) / 2, screenH / 2 - 35, font1,
            view.stats.splitBonus ? Color {250, 204, 21, 255} : Color {74, 222, 128, 255});

        if (!view.stats.splitBonus && view.stats.thresholdBonus > 0) {
            const auto overshoot = (view.stats.claimedPercent > view.stats.targetPercent)
                ? (view.stats.claimedPercent - view.stats.targetPercent)
                : 0U;
            const std::string bonusText = "+" + std::to_string(view.stats.thresholdBonus) + " THRESHOLD BONUS (+"
                + std::to_string(overshoot) + "%)";
            const int fontB = 20;
            const int wb = MeasureText(bonusText.c_str(), fontB);
            DrawText(bonusText.c_str(), (screenW - wb) / 2, screenH / 2, fontB, Color {250, 204, 21, 255});
        }

        const std::string text2 = view.stats.splitBonus
            ? ("Multiplier Increased to " + std::to_string(view.stats.multiplier) + "X! Press [Space]")
            : "Press [Space] for Next Level";
        const int font2 = 18;
        const int w2 = MeasureText(text2.c_str(), font2);
        const int y2
            = (!view.stats.splitBonus && view.stats.thresholdBonus > 0) ? (screenH / 2 + 28) : (screenH / 2 + 15);
        DrawText(text2.c_str(), (screenW - w2) / 2, y2, font2, Color {243, 244, 246, 255});
    } else if (view.state == GameState::NameEntry) {
        drawNameEntry(view.nameEntry, view.stats);
    } else if (view.state == GameState::HallOfFame) {
        drawHallOfFame(view.highScoreTable, false);
    } else if (view.state == GameState::GameOver) {
        drawHallOfFame(view.highScoreTable, true);
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

    const std::string rankStr
        = "NEW RECORD! RANK #" + std::to_string(entry.rank) + " - SCORE: " + std::to_string(stats.score);
    const int fontSub = 18;
    const int wSub = MeasureText(rankStr.c_str(), fontSub);
    DrawText(rankStr.c_str(), (screenW - wSub) / 2, screenH / 2 - 95, fontSub, Color {99, 179, 237, 255});

    const char* prompt = "ENTER YOUR INITIALS";
    const int fontPrompt = 16;
    const int wPrompt = MeasureText(prompt, fontPrompt);
    DrawText(prompt, (screenW - wPrompt) / 2, screenH / 2 - 60, fontPrompt, Color {243, 244, 246, 255});

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

void RaylibRenderer::drawHallOfFame(const HighScoreTable* table, bool isGameOver) noexcept
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
        const auto& entries = table->getEntries();
        const std::size_t maxRows = std::min(entries.size(), static_cast<std::size_t>(7));

        for (std::size_t i {0}; i < maxRows; ++i) {
            const auto& e = entries[i];
            Color rowColor {148, 163, 184, 255};
            if (i == 0) {
                rowColor = Color {250, 204, 21, 255}; // Gold
            } else if (i == 1) {
                rowColor = Color {226, 232, 240, 255}; // Silver
            } else if (i == 2) {
                rowColor = Color {245, 158, 11, 255}; // Bronze
            }

            char rowBuf[64];
            const char* modeStr = (e.mode == GameMode::Classic) ? "CLASSIC" : "MODERN";
            std::snprintf(rowBuf, sizeof(rowBuf), "%2zu.      %-4s    %8u      %02u     %-7s", i + 1,
                e.initials.c_str(), e.score, static_cast<unsigned>(e.level), modeStr);

            const int wRow = MeasureText(rowBuf, fontRow);
            DrawText(rowBuf, (screenW - wRow) / 2, curY, fontRow, rowColor);
            curY += 20;
        }
    }

    curY += 15;
    const char* prompt = "Press [R] or [SPACE] to Play Again";
    const int fontPrompt = 16;
    const int wPrompt = MeasureText(prompt, fontPrompt);
    DrawText(prompt, (screenW - wPrompt) / 2, curY, fontPrompt, Color {243, 244, 246, 255});
}

} // namespace qix::raylib
