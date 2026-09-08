#include "TuiRenderer.h"
#include "HighScoreTable.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <cstdlib>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace qix::tui {

namespace {

    constexpr std::uint8_t kDotMask[4][2] = {
        {0x01, 0x08}, // row 0: dot 1, dot 4
        {0x02, 0x10}, // row 1: dot 2, dot 5
        {0x04, 0x20}, // row 2: dot 3, dot 6
        {0x40, 0x80}, // row 3: dot 7, dot 8
    };

    void appendBrailleUtf8(std::string& out, std::uint8_t mask) noexcept
    {
        out.push_back(static_cast<char>(0xE2));
        out.push_back(static_cast<char>(0xA0 | ((mask >> 6) & 0x03)));
        out.push_back(static_cast<char>(0x80 | (mask & 0x3F)));
    }

    template <typename PlotFn> void bresenhamLine(int x0, int y0, int x1, int y1, PlotFn&& plot) noexcept
    {
        const int dx = std::abs(x1 - x0);
        const int dy = -std::abs(y1 - y0);
        const int sx = (x0 < x1) ? 1 : -1;
        const int sy = (y0 < y1) ? 1 : -1;
        int err = dx + dy;

        while (true) {
            plot(x0, y0);
            if (x0 == x1 && y0 == y1) {
                break;
            }
            const int e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }

    struct Rgb {
        std::uint8_t r {0};
        std::uint8_t g {0};
        std::uint8_t b {0};
    };

    Rgb hsvToRgb(double h, double s, double v) noexcept
    {
        h = std::fmod(h, 360.0);
        if (h < 0.0) {
            h += 360.0;
        }

        const double c = v * s;
        const double x = c * (1.0 - std::abs(std::fmod(h / 60.0, 2.0) - 1.0));
        const double m = v - c;

        double r1 = 0.0;
        double g1 = 0.0;
        double b1 = 0.0;

        if (h < 60.0) {
            r1 = c;
            g1 = x;
        } else if (h < 120.0) {
            r1 = x;
            g1 = c;
        } else if (h < 180.0) {
            g1 = c;
            b1 = x;
        } else if (h < 240.0) {
            g1 = x;
            b1 = c;
        } else if (h < 300.0) {
            r1 = x;
            b1 = c;
        } else {
            r1 = c;
            b1 = x;
        }

        return Rgb {static_cast<std::uint8_t>(std::clamp(std::round((r1 + m) * 255.0), 0.0, 255.0)),
            static_cast<std::uint8_t>(std::clamp(std::round((g1 + m) * 255.0), 0.0, 255.0)),
            static_cast<std::uint8_t>(std::clamp(std::round((b1 + m) * 255.0), 0.0, 255.0))};
    }

    void appendTruecolor(std::string& out, std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept
    {
        char buf[32];
        const int len = std::snprintf(buf, sizeof(buf), "\033[38;2;%u;%u;%um", r, g, b);
        if (len > 0) {
            out.append(buf, static_cast<std::size_t>(len));
        }
    }

    struct BrailleCell {
        std::uint8_t dots {0};
        char specialChar {0};
        std::uint8_t r {0};
        std::uint8_t g {0};
        std::uint8_t b {0};
        bool isRgb {false};
        const char* ansiColor {""};
        std::uint8_t priority {0};
    };

    void appendProgressBar(
        std::string& frame, double claimed, std::uint16_t target, int termCols, bool truecolor) noexcept
    {
        static constexpr const char* kFracs[8] = {
            "",
            "\xe2\x96\x8f", // 1/8 ▏
            "\xe2\x96\x8e", // 2/8 ▎
            "\xe2\x96\x8d", // 3/8 ▍
            "\xe2\x96\x8c", // 4/8 ▌
            "\xe2\x96\x8b", // 5/8 ▋
            "\xe2\x96\x8a", // 6/8 ▊
            "\xe2\x96\x89" // 7/8 ▉
        };

        const int barWidth = std::clamp(termCols - 45, 16, 40);
        const int totalEighths = static_cast<int>(std::round((claimed / 100.0) * barWidth * 8.0));
        const int fullChars = totalEighths / 8;
        const int fracIdx = totalEighths % 8;
        const int targetChar = static_cast<int>(std::round((static_cast<double>(target) / 100.0) * barWidth));

        const std::string fillCol = truecolor
            ? ((claimed < static_cast<double>(target)) ? "\033[38;2;0;220;240m" : "\033[38;2;255;215;0m")
            : ((claimed < static_cast<double>(target)) ? "\033[1;36m" : "\033[1;33m");
        const std::string emptyCol = truecolor ? "\033[38;2;60;70;90m" : "\033[2;37m";
        const std::string targetCol = truecolor ? "\033[38;2;255;90;90m" : "\033[1;31m";

        frame += "Territory: [";
        for (int i = 0; i < barWidth; ++i) {
            if (i < fullChars) {
                frame += fillCol;
                frame += "\xe2\x96\x88"; // █
            } else if (i == fullChars && fracIdx > 0) {
                frame += fillCol;
                frame += kFracs[fracIdx];
            } else if (i == targetChar && i >= fullChars) {
                frame += targetCol;
                frame += "\xe2\x94\x82"; // │
            } else {
                frame += emptyCol;
                frame += "\xe2\x96\x91"; // ░
            }
        }
        frame += "\033[0m] ";

        char pctBuf[80];
        if (claimed < static_cast<double>(target)) {
            std::snprintf(pctBuf, sizeof(pctBuf), "\033[1;36m%4.1f%%\033[0m / %u%% Target", claimed, target);
        } else {
            const int bonus = static_cast<int>(claimed) - static_cast<int>(target);
            std::snprintf(pctBuf, sizeof(pctBuf), "\033[1;33m%4.1f%%\033[0m / %u%% \033[1;32m(MET! +%d%% Bonus)\033[0m",
                claimed, target, bonus);
        }
        frame += pctBuf;
        frame += "\n";
    }

} // namespace

#ifndef _WIN32
static struct termios s_origTermios;
static bool s_hasOrigTermios = false;
#endif

TuiRenderer::TuiRenderer() noexcept = default;

TuiRenderer::~TuiRenderer() noexcept
{
    shutdown();
}

void TuiRenderer::init() noexcept
{
    if (m_initialized) {
        return;
    }

#ifdef _WIN32
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
#else
    if (!s_hasOrigTermios) {
        tcgetattr(STDIN_FILENO, &s_origTermios);
        s_hasOrigTermios = true;
    }

    struct termios raw = s_origTermios;
    raw.c_lflag &= ~(ECHO | ICANON);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
#endif

    // Hide cursor and clear screen
    std::cout << "\033[?25l\033[2J\033[H" << std::flush;
    m_initialized = true;
}

void TuiRenderer::shutdown() noexcept
{
    if (!m_initialized) {
        return;
    }

    // Show cursor and reset colors
    std::cout << "\033[?25h\033[0m\n" << std::flush;

#ifndef _WIN32
    if (s_hasOrigTermios) {
        tcsetattr(STDIN_FILENO, TCSANOW, &s_origTermios);
    }
#endif

    m_initialized = false;
}

void TuiRenderer::clearScreen() noexcept
{
    std::cout << "\033[2J\033[H" << std::flush;
}

TerminalSize TuiRenderer::queryTerminalSize() noexcept
{
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        const int cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        const int rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        if (cols >= 20 && rows >= 10) {
            return TerminalSize {cols, rows};
        }
    }
#else
    struct winsize ws { };
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col >= 20 && ws.ws_row >= 10) {
        return TerminalSize {static_cast<int>(ws.ws_col), static_cast<int>(ws.ws_row)};
    }
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col >= 20 && ws.ws_row >= 10) {
        return TerminalSize {static_cast<int>(ws.ws_col), static_cast<int>(ws.ws_row)};
    }
#endif

    const char* colEnv = std::getenv("COLUMNS");
    const char* rowEnv = std::getenv("LINES");
    if (colEnv != nullptr && rowEnv != nullptr) {
        const int c = std::atoi(colEnv);
        const int r = std::atoi(rowEnv);
        if (c >= 20 && r >= 10) {
            return TerminalSize {c, r};
        }
    }

    return TerminalSize {80, 24};
}

std::pair<std::int32_t, std::int32_t> TuiRenderer::computePlayfieldDimensions(bool /*brailleMode*/) noexcept
{
    const auto term = queryTerminalSize();
    // Vertical overhead: HUD (3) + top border (1) + bottom border (1) + controls (1) + margin (1) = 7 lines
    const int charRows = std::max(10, term.rows - 7);
    // Horizontal overhead: left border (1) + right border (1) + side margin (2) = 4 chars
    const int charCols = std::max(20, term.cols - 4);

    return {static_cast<std::int32_t>(charCols * 2), static_cast<std::int32_t>(charRows * 4)};
}

bool TuiRenderer::checkAndHandleResize() noexcept
{
    const auto current = queryTerminalSize();
    if (m_lastTermSize.cols == 0 && m_lastTermSize.rows == 0) {
        m_lastTermSize = current;
        return false;
    }
    if (m_lastTermSize.cols != current.cols || m_lastTermSize.rows != current.rows) {
        m_lastTermSize = current;
        clearScreen();
        return true;
    }
    return false;
}

void TuiRenderer::setBrailleMode(bool enabled) noexcept
{
    m_brailleMode = enabled;
}

bool TuiRenderer::isBrailleMode() const noexcept
{
    return m_brailleMode;
}

void TuiRenderer::toggleBrailleMode() noexcept
{
    m_brailleMode = !m_brailleMode;
}

void TuiRenderer::setTruecolor(bool enabled) noexcept
{
    m_truecolor = enabled;
}

bool TuiRenderer::isTruecolor() const noexcept
{
    return m_truecolor;
}

void TuiRenderer::toggleTruecolor() noexcept
{
    m_truecolor = !m_truecolor;
}

void TuiRenderer::render(const GameView& view, std::uint32_t delayMs) noexcept
{
    if (!view.playfield) {
        return;
    }

    if (m_lastTermSize.cols == 0 && m_lastTermSize.rows == 0) {
        m_lastTermSize = queryTerminalSize();
    }

    ++m_colorCycle;

    // Move cursor to top-left
    std::string frame = "\033[H";

    std::string stateStr = "READY";
    if (view.state == GameState::Playing) {
        stateStr = "PLAYING";
    } else if (view.state == GameState::LevelComplete) {
        if (view.stats.splitBonus) {
            stateStr = "\033[1;32mQIX SPLIT BONUS!\033[0m";
        } else {
            stateStr = "\033[1;32mLEVEL COMPLETE\033[0m";
        }
    } else if (view.state == GameState::NameEntry) {
        stateStr = "\033[1;33mENTER INITIALS\033[0m";
    } else if (view.state == GameState::HallOfFame) {
        stateStr = "\033[1;36mHALL OF FAME\033[0m";
    } else if (view.state == GameState::GameOver) {
        stateStr = "\033[1;31mGAME OVER\033[0m";
    }

    std::string modeStr = m_brailleMode ? "Braille (Hi-Res)" : "ASCII";
    if (m_truecolor) {
        modeStr += " [RGB]";
    }

    // 1. HUD Header - Line 1: Title, Mode, and State
    frame
        += "\033[1;36m=== QIX C++17 ARCADE ===\033[0m | Mode: \033[1;36m" + modeStr + "\033[0m | [" + stateStr + "]\n";

    // HUD Header - Line 2: Gameplay Stats
    frame += "Score: \033[1;33m" + std::to_string(view.stats.score) + "\033[0m | ";
    frame += "High: \033[1;33m" + std::to_string(view.stats.highScore) + "\033[0m | ";
    frame += "Lives: \033[1;31m" + std::to_string(view.stats.lives) + "\033[0m | ";
    frame += "Level: \033[1;35m" + std::to_string(view.stats.level) + "\033[0m | ";
    if (view.stats.multiplier > 1) {
        frame += "Mult: \033[1;33m" + std::to_string(view.stats.multiplier) + "x\033[0m | ";
    }
    const auto secondsRemaining = (view.stats.timeRemainingMs + 999U) / 1000U;
    const std::string timeColor = (view.stats.timeUp || secondsRemaining <= 10U)
        ? "\033[1;31m"
        : ((secondsRemaining <= 20U) ? "\033[1;33m" : "\033[1;32m");
    frame += "Time: " + timeColor + std::to_string(secondsRemaining) + "s\033[0m | ";
    frame += "Delay: \033[1;36m" + std::to_string(delayMs) + "ms\033[0m\n";

    // HUD Header - Line 3: Real-Time Unicode Territory Progress Bar
    const double claimedPercent = (view.playfield && view.playfield->getInteriorCount() > 0)
        ? (static_cast<double>(view.playfield->getClaimedCount()) * 100.0
            / static_cast<double>(view.playfield->getInteriorCount()))
        : static_cast<double>(view.stats.claimedPercent);
    appendProgressBar(frame, claimedPercent, view.stats.targetPercent, m_lastTermSize.cols, m_truecolor);

    if (view.state == GameState::NameEntry) {
        renderNameEntry(frame, view.nameEntry, view.stats);
        std::cout << frame << std::flush;
        return;
    }
    if (view.state == GameState::HallOfFame || view.state == GameState::GameOver) {
        renderHallOfFame(frame, view.highScoreTable, view.state == GameState::GameOver);
        std::cout << frame << std::flush;
        return;
    }

    // 2. Playfield Grid (Braille Sub-Pixel or Classic ASCII)
    if (m_brailleMode) {
        renderBraillePlayfield(frame, view);
    } else {
        renderAsciiPlayfield(frame, view);
    }

    // 3. Controls Legend
    frame += "\033[2mControls: [WASD/Arrows] Move | [Space] Slow | [F] Fast | [X] Border | [B] Braille/ASCII | [T] RGB "
             "| [-/+] Speed | [R] Reset | [Q] Quit\033[0m\n";

    std::cout << frame << std::flush;
}

void TuiRenderer::renderBraillePlayfield(std::string& frame, const GameView& view) noexcept
{
    const auto width = view.playfield->getWidth();
    const auto height = view.playfield->getHeight();

    const std::int32_t cols = (width + 1) / 2;
    const std::int32_t rows = (height + 3) / 4;

    std::vector<BrailleCell> grid(static_cast<std::size_t>(cols * rows));

    // 1. Plot Playfield Cells (Borders, Claimed Areas, Active Stix)
    for (std::int32_t y {0}; y < height; ++y) {
        for (std::int32_t x {0}; x < width; ++x) {
            const auto state = view.playfield->getCell(x, y);
            if (state == CellState::Empty) {
                continue;
            }

            const int cx = x / 2;
            const int cy = y / 4;
            auto& cell = grid[static_cast<std::size_t>(cy * cols + cx)];
            const std::uint8_t dot = kDotMask[y % 4][x % 2];
            cell.dots |= dot;

            if (state == CellState::Border) {
                if (cell.priority < 2) {
                    cell.priority = 2;
                    if (m_truecolor) {
                        cell.isRgb = true;
                        cell.r = 40;
                        cell.g = 90;
                        cell.b = 230;
                    } else {
                        cell.isRgb = false;
                        cell.ansiColor = "\033[1;34m"; // Bright blue
                    }
                }
            } else if (state == CellState::ActiveStix) {
                if (cell.priority < 3) {
                    cell.priority = 3;
                    if (m_truecolor) {
                        cell.isRgb = true;
                        cell.r = 255;
                        cell.g = 255;
                        cell.b = 255;
                    } else {
                        cell.isRgb = false;
                        cell.ansiColor = "\033[1;37m"; // Bright white
                    }
                }
            } else if (state == CellState::ClaimedSlow) {
                if (cell.priority < 1) {
                    cell.priority = 1;
                    if (m_truecolor) {
                        cell.isRgb = true;
                        cell.r = 0;
                        cell.g = 210;
                        cell.b = 230;
                    } else {
                        cell.isRgb = false;
                        cell.ansiColor = "\033[0;36m"; // Cyan
                    }
                }
            } else if (state == CellState::ClaimedFast) {
                if (cell.priority < 1) {
                    cell.priority = 1;
                    if (m_truecolor) {
                        cell.isRgb = true;
                        cell.r = 30;
                        cell.g = 220;
                        cell.b = 100;
                    } else {
                        cell.isRgb = false;
                        cell.ansiColor = "\033[0;32m"; // Green
                    }
                }
            }
        }
    }

    // 2. Active Stix Trail (ensure high fidelity along active trail)
    for (const auto& pt : view.stixTrail) {
        if (pt.x >= 0 && pt.x < width && pt.y >= 0 && pt.y < height) {
            const int cx = pt.x / 2;
            const int cy = pt.y / 4;
            auto& cell = grid[static_cast<std::size_t>(cy * cols + cx)];
            cell.dots |= kDotMask[pt.y % 4][pt.x % 2];
            if (cell.priority < 3) {
                cell.priority = 3;
                if (m_truecolor) {
                    cell.isRgb = true;
                    cell.r = 255;
                    cell.g = 255;
                    cell.b = 255;
                } else {
                    cell.isRgb = false;
                    cell.ansiColor = "\033[1;37m";
                }
            }
        }
    }

    // 3. Qix Ribbons (Sub-Pixel Vector Bresenham Line Rasterization with 24-bit Truecolor Neon Cycling)
    for (const auto& ribbon : view.qixRibbons) {
        const auto totalSegs = ribbon.size();
        for (std::size_t segIdx {0}; segIdx < totalSegs; ++segIdx) {
            const auto& seg = ribbon[segIdx];
            const double hue
                = std::fmod(m_colorCycle * 6.0 + segIdx * (360.0 / std::max<std::size_t>(1, totalSegs)), 360.0);
            const double sat = (segIdx == 0) ? 0.70 : 0.95;
            const double val = (segIdx == 0)
                ? 1.0
                : std::max(0.35, 1.0 - 0.55 * (static_cast<double>(segIdx) / static_cast<double>(totalSegs)));
            const Rgb segRgb = hsvToRgb(hue, sat, val);
            const char* segAnsi = (segIdx == 0) ? "\033[1;31m" : ((segIdx < 3) ? "\033[1;35m" : "\033[0;35m");

            bresenhamLine(seg.start.x, seg.start.y, seg.end.x, seg.end.y, [&](int lx, int ly) {
                if (lx >= 0 && lx < width && ly >= 0 && ly < height) {
                    const int cx = lx / 2;
                    const int cy = ly / 4;
                    auto& cell = grid[static_cast<std::size_t>(cy * cols + cx)];
                    cell.dots |= kDotMask[ly % 4][lx % 2];
                    if (cell.priority < 4) {
                        cell.priority = 4;
                        if (m_truecolor) {
                            cell.isRgb = true;
                            cell.r = segRgb.r;
                            cell.g = segRgb.g;
                            cell.b = segRgb.b;
                        } else {
                            cell.isRgb = false;
                            cell.ansiColor = segAnsi;
                        }
                    }
                }
            });
        }
    }

    // 4. Sparx & Super Sparx
    if (!view.sparxList.empty()) {
        for (const auto& sp : view.sparxList) {
            if (sp.position.x >= 0 && sp.position.x < width && sp.position.y >= 0 && sp.position.y < height) {
                const int cx = sp.position.x / 2;
                const int cy = sp.position.y / 4;
                auto& cell = grid[static_cast<std::size_t>(cy * cols + cx)];
                cell.specialChar = sp.isSuper ? 'S' : '$';
                cell.priority = 5;
                if (m_truecolor) {
                    cell.isRgb = true;
                    if (sp.isSuper) {
                        cell.r = 0;
                        cell.g = 255;
                        cell.b = 255;
                    } else {
                        cell.r = 255;
                        cell.g = 50;
                        cell.b = 220;
                    }
                } else {
                    cell.isRgb = false;
                    cell.ansiColor = sp.isSuper ? "\033[1;36m" : "\033[1;35m";
                }
            }
        }
    } else {
        for (const auto& pos : view.sparxPositions) {
            if (pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height) {
                const int cx = pos.x / 2;
                const int cy = pos.y / 4;
                auto& cell = grid[static_cast<std::size_t>(cy * cols + cx)];
                cell.specialChar = '$';
                cell.priority = 5;
                if (m_truecolor) {
                    cell.isRgb = true;
                    cell.r = 255;
                    cell.g = 50;
                    cell.b = 220;
                } else {
                    cell.isRgb = false;
                    cell.ansiColor = "\033[1;35m";
                }
            }
        }
    }

    // 5. Fuse
    if (view.fusePos.has_value()) {
        const auto fp = view.fusePos.value();
        if (fp.x >= 0 && fp.x < width && fp.y >= 0 && fp.y < height) {
            const int cx = fp.x / 2;
            const int cy = fp.y / 4;
            auto& cell = grid[static_cast<std::size_t>(cy * cols + cx)];
            cell.specialChar = '!';
            cell.priority = 5;
            if (m_truecolor) {
                cell.isRgb = true;
                cell.r = 255;
                cell.g = 30;
                cell.b = 30;
            } else {
                cell.isRgb = false;
                cell.ansiColor = "\033[1;31m";
            }
        }
    }

    // 6. Player Marker
    if (view.markerPos.x >= 0 && view.markerPos.x < width && view.markerPos.y >= 0 && view.markerPos.y < height) {
        const int cx = view.markerPos.x / 2;
        const int cy = view.markerPos.y / 4;
        auto& cell = grid[static_cast<std::size_t>(cy * cols + cx)];
        cell.specialChar = '@';
        cell.priority = 6;
        if (m_truecolor) {
            cell.isRgb = true;
            cell.r = 255;
            cell.g = 220;
            cell.b = 40;
        } else {
            cell.isRgb = false;
            cell.ansiColor = "\033[1;33m";
        }
    }

    // 7. Output Rendered Braille Frame with Border Framing
    const std::string borderCol = m_truecolor ? "\033[38;2;40;90;230m" : "\033[1;34m";
    frame += borderCol + "┌";
    for (int cx {0}; cx < cols; ++cx) {
        frame += "─";
    }
    frame += "┐\033[0m\n";

    for (std::int32_t cy {0}; cy < rows; ++cy) {
        frame += borderCol + "│\033[0m";
        for (std::int32_t cx {0}; cx < cols; ++cx) {
            const auto& cell = grid[static_cast<std::size_t>(cy * cols + cx)];
            if (cell.specialChar != 0) {
                if (cell.isRgb) {
                    appendTruecolor(frame, cell.r, cell.g, cell.b);
                } else {
                    frame += cell.ansiColor;
                }
                frame += cell.specialChar;
                frame += "\033[0m";
            } else if (cell.dots != 0) {
                if (cell.isRgb) {
                    appendTruecolor(frame, cell.r, cell.g, cell.b);
                } else {
                    frame += cell.ansiColor;
                }
                appendBrailleUtf8(frame, cell.dots);
                frame += "\033[0m";
            } else {
                frame += " ";
            }
        }
        frame += borderCol + "│\033[0m\n";
    }

    frame += borderCol + "└";
    for (int cx {0}; cx < cols; ++cx) {
        frame += "─";
    }
    frame += "┘\033[0m\n";
}

void TuiRenderer::renderAsciiPlayfield(std::string& frame, const GameView& view) noexcept
{
    const auto width = view.playfield->getWidth();
    const auto height = view.playfield->getHeight();

    const auto term = queryTerminalSize();
    const int maxCols = std::max(20, term.cols - 4);
    const int maxRows = std::max(10, term.rows - 7);

    const std::int32_t stepX = std::max(1, (width + maxCols - 1) / maxCols);
    const std::int32_t stepY = std::max(1, (height + maxRows - 1) / maxRows);
    const std::int32_t cols = (width + stepX - 1) / stepX;

    const std::string borderCol = m_truecolor ? "\033[38;2;40;90;230m" : "\033[1;34m";

    // Top border
    frame += borderCol + "┌";
    for (std::int32_t cx {0}; cx < cols; ++cx) {
        frame += "─";
    }
    frame += "┐\033[0m\n";

    for (std::int32_t y {0}; y < height; y += stepY) {
        frame += borderCol + "│\033[0m";
        for (std::int32_t x {0}; x < width; x += stepX) {
            Point p {x, y};

            // Player Marker
            if (p == view.markerPos) {
                frame += "\033[1;33m@\033[0m";
                continue;
            }

            // Fuse
            if (view.fusePos.has_value() && p == view.fusePos.value()) {
                frame += "\033[1;31m!\033[0m";
                continue;
            }

            // Sparx
            bool isSparx = false;
            bool isSuper = false;
            if (!view.sparxList.empty()) {
                for (const auto& sp : view.sparxList) {
                    if (p == sp.position) {
                        isSparx = true;
                        isSuper = sp.isSuper;
                        break;
                    }
                }
            } else {
                for (const auto& sp : view.sparxPositions) {
                    if (p == sp) {
                        isSparx = true;
                        break;
                    }
                }
            }
            if (isSparx) {
                frame += isSuper ? "\033[1;36mS\033[0m" : "\033[1;35m$\033[0m";
                continue;
            }

            // Qix segments
            bool isQix = false;
            Rgb qixRgb {};
            const char* qixAnsi = "\033[1;31m";
            for (const auto& ribbon : view.qixRibbons) {
                const auto totalSegs = ribbon.size();
                for (std::size_t segIdx {0}; segIdx < totalSegs; ++segIdx) {
                    const auto& seg = ribbon[segIdx];
                    const auto minX = std::min(seg.start.x, seg.end.x);
                    const auto maxX = std::max(seg.start.x, seg.end.x);
                    const auto minY = std::min(seg.start.y, seg.end.y);
                    const auto maxY = std::max(seg.start.y, seg.end.y);
                    if (x >= minX && x <= maxX && y >= minY && y <= maxY) {
                        isQix = true;
                        const double hue = std::fmod(
                            m_colorCycle * 6.0 + segIdx * (360.0 / std::max<std::size_t>(1, totalSegs)), 360.0);
                        const double sat = (segIdx == 0) ? 0.70 : 0.95;
                        const double val = (segIdx == 0)
                            ? 1.0
                            : std::max(
                                0.35, 1.0 - 0.55 * (static_cast<double>(segIdx) / static_cast<double>(totalSegs)));
                        qixRgb = hsvToRgb(hue, sat, val);
                        qixAnsi = (segIdx == 0) ? "\033[1;31m" : ((segIdx < 3) ? "\033[1;35m" : "\033[0;35m");
                        break;
                    }
                }
                if (isQix) {
                    break;
                }
            }
            if (isQix) {
                if (m_truecolor) {
                    appendTruecolor(frame, qixRgb.r, qixRgb.g, qixRgb.b);
                    frame += "X\033[0m";
                } else {
                    frame += qixAnsi;
                    frame += "X\033[0m";
                }
                continue;
            }

            // Stix trail
            const auto state = view.playfield->getCell(x, y);
            if (state == CellState::ActiveStix) {
                frame += "\033[1;37m*\033[0m";
            } else if (state == CellState::Border) {
                frame += "\033[1;34m#\033[0m";
            } else if (state == CellState::ClaimedSlow) {
                frame += "\033[0;36m.\033[0m";
            } else if (state == CellState::ClaimedFast) {
                frame += "\033[0;32m,\033[0m";
            } else {
                frame += " ";
            }
        }
        frame += borderCol + "│\033[0m\n";
    }

    // Bottom border
    frame += borderCol + "└";
    for (std::int32_t cx {0}; cx < cols; ++cx) {
        frame += "─";
    }
    frame += "┘\033[0m\n";
}

PlayerCommand TuiRenderer::pollInput(TuiAction& action) noexcept
{
    PlayerCommand cmd {};
    action = TuiAction::None;

    const bool resized = checkAndHandleResize();

    int ch = -1;

#ifdef _WIN32
    if (_kbhit()) {
        ch = _getch();
        if (ch == 224) { // Extended key
            ch = _getch();
            switch (ch) {
            case 72:
                cmd.direction = Direction::Up;
                break;
            case 80:
                cmd.direction = Direction::Down;
                break;
            case 75:
                cmd.direction = Direction::Left;
                break;
            case 77:
                cmd.direction = Direction::Right;
                break;
            default:
                break;
            }
            if (action == TuiAction::None && resized) {
                action = TuiAction::Resize;
            }
            return cmd;
        }
    }
#else
    char buf[8] {0};
    const auto n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
    if (n > 0) {
        if (buf[0] == '\033' && n >= 3 && buf[1] == '[') {
            switch (buf[2]) {
            case 'A':
                cmd.direction = Direction::Up;
                break;
            case 'B':
                cmd.direction = Direction::Down;
                break;
            case 'C':
                cmd.direction = Direction::Right;
                break;
            case 'D':
                cmd.direction = Direction::Left;
                break;
            default:
                break;
            }
            return cmd;
        }
        ch = static_cast<unsigned char>(buf[0]);
    }
#endif

    if (ch != -1) {
        switch (ch) {
        case 'q':
        case 'Q':
            action = TuiAction::Quit;
            break;
        case 'r':
        case 'R':
            action = TuiAction::Restart;
            break;
        case '-':
        case '_':
        case '[':
            action = TuiAction::SpeedDown;
            break;
        case '+':
        case '=':
        case ']':
            action = TuiAction::SpeedUp;
            break;
        case 'w':
        case 'W':
            cmd.direction = Direction::Up;
            break;
        case 's':
        case 'S':
            cmd.direction = Direction::Down;
            break;
        case 'a':
        case 'A':
            cmd.direction = Direction::Left;
            break;
        case 'd':
        case 'D':
            cmd.direction = Direction::Right;
            break;
        case ' ':
            cmd.drawMode = DrawMode::Slow;
            break;
        case 'f':
        case 'F':
            cmd.drawMode = DrawMode::Fast;
            break;
        case 'x':
        case 'X':
            action = TuiAction::DisengageDraw;
            break;
        case 'b':
        case 'B':
            action = TuiAction::ToggleBraille;
            break;
        case 't':
        case 'T':
            action = TuiAction::ToggleTruecolor;
            break;
        case '\n':
        case '\r':
            action = TuiAction::Confirm;
            break;
        default:
            break;
        }
    }

    if (action == TuiAction::None && resized) {
        action = TuiAction::Resize;
    }

    return cmd;
}

void TuiRenderer::renderNameEntry(std::string& frame, const NameEntryState& entry, const GameStats& stats) noexcept
{
    frame += "\n";
    frame += "  \033[1;33m+------------------------------------------------------------+\033[0m\n";
    frame += "  \033[1;33m|                 * ARCADE HALL OF FAME *                    |\033[0m\n";
    char rankBuf[128];
    std::snprintf(rankBuf, sizeof(rankBuf), "  |   NEW HIGH SCORE RECORD! RANK #%zu - SCORE: %-15u  |\n", entry.rank,
        stats.score);
    frame += rankBuf;
    frame += "  |                                                            |\n";
    frame += "  |                 ENTER YOUR 3-LETTER INITIALS               |\n";
    frame += "  |                                                            |\n";

    std::string slot0 = (entry.cursorIndex == 0) ? ("\033[1;33m[" + std::string(1, entry.initials[0]) + "]\033[0m")
                                                 : (" " + std::string(1, entry.initials[0]) + " ");
    std::string slot1 = (entry.cursorIndex == 1) ? ("\033[1;33m[" + std::string(1, entry.initials[1]) + "]\033[0m")
                                                 : (" " + std::string(1, entry.initials[1]) + " ");
    std::string slot2 = (entry.cursorIndex == 2) ? ("\033[1;33m[" + std::string(1, entry.initials[2]) + "]\033[0m")
                                                 : (" " + std::string(1, entry.initials[2]) + " ");

    frame += "  |                           " + slot0 + "  " + slot1 + "  " + slot2 + "                        |\n";
    frame += "  |                                                            |\n";
    frame += "  \033[1;36m|   [W/S] Change Letter   [A/D] Move Slot   [Space/Enter] OK |\033[0m\n";
    frame += "  \033[1;33m+------------------------------------------------------------+\033[0m\n";
}

void TuiRenderer::renderHallOfFame(std::string& frame, const HighScoreTable* table, bool isGameOver) noexcept
{
    frame += "\n";
    if (isGameOver) {
        frame += "  \033[1;31m========================= GAME OVER =========================\033[0m\n";
    }
    frame += "  \033[1;33m+------------------------------------------------------------+\033[0m\n";
    frame += "  \033[1;33m|                 * ARCADE HALL OF FAME *                    |\033[0m\n";
    frame += "  \033[1;36m|  RANK   NAME         SCORE          LEVEL      MODE        |\033[0m\n";
    frame += "  \033[1;36m|  --------------------------------------------------------  |\033[0m\n";

    if (table != nullptr) {
        const auto& entries = table->getEntries();
        const std::size_t maxRows = std::min(entries.size(), static_cast<std::size_t>(8));

        for (std::size_t i {0}; i < maxRows; ++i) {
            const auto& e = entries[i];
            const char* modeStr = (e.mode == GameMode::Classic) ? "CLASSIC" : "MODERN";
            char rowBuf[128];
            std::snprintf(rowBuf, sizeof(rowBuf), "  |  %2zu.    %-4s        %8u            %2u      %-7s |\n", i + 1,
                e.initials.c_str(), e.score, static_cast<unsigned>(e.level), modeStr);
            frame += rowBuf;
        }
    }

    frame += "  |                                                            |\n";
    frame += "  \033[1;32m|              Press [R] or [Space] to Play Again            |\033[0m\n";
    frame += "  \033[1;33m+------------------------------------------------------------+\033[0m\n";
}

} // namespace qix::tui
