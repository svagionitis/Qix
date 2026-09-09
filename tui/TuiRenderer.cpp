#include "TuiRenderer.h"
#include "GamePresenter.h"
#include "HighScoreTable.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <cstring>
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

    void appendTruecolor(std::string& out, std::uint8_t r, std::uint8_t g, std::uint8_t b) noexcept
    {
        char buf[32];
        const int len = std::snprintf(buf, sizeof(buf), "\033[38;2;%u;%u;%um", r, g, b);
        if (len > 0) {
            out.append(buf, static_cast<std::size_t>(len));
        }
    }

    std::string appendTruecolorStr(const PaletteColor& c) noexcept
    {
        char buf[32];
        const int len = std::snprintf(buf, sizeof(buf), "\033[38;2;%u;%u;%um", c.r, c.g, c.b);
        return (len > 0) ? std::string(buf, static_cast<std::size_t>(len)) : std::string {};
    }

    struct ThemeAnsi {
        const char* border;
        const char* claimedSlow;
        const char* claimedFast;
        const char* activeStix;
        const char* marker;
        const char* sparx;
        const char* superSparx;
        const char* fuse;
    };

    constexpr ThemeAnsi getThemeAnsi(PaletteId id) noexcept
    {
        switch (id) {
        case PaletteId::Classic:
            return {"\033[1;34m", "\033[0;36m", "\033[0;32m", "\033[1;37m", "\033[1;33m", "\033[1;35m", "\033[1;36m",
                "\033[1;31m"};
        case PaletteId::Synthwave:
            return {"\033[1;36m", "\033[1;35m", "\033[0;34m", "\033[1;37m", "\033[1;33m", "\033[1;35m", "\033[1;36m",
                "\033[1;31m"};
        case PaletteId::Amber:
            return {"\033[1;33m", "\033[0;33m", "\033[2;33m", "\033[1;37m", "\033[1;33m", "\033[1;33m", "\033[1;37m",
                "\033[1;31m"};
        case PaletteId::Green:
            return {"\033[1;32m", "\033[0;32m", "\033[2;32m", "\033[1;37m", "\033[1;32m", "\033[1;32m", "\033[1;37m",
                "\033[1;31m"};
        }
        return {"\033[1;34m", "\033[0;36m", "\033[0;32m", "\033[1;37m", "\033[1;33m", "\033[1;35m", "\033[1;36m",
            "\033[1;31m"};
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

    [[nodiscard]] int visibleWidth(const std::string& str) noexcept
    {
        int width = 0;
        bool inEscape = false;
        for (std::size_t i {0}; i < str.size(); ++i) {
            const char c = str[i];
            if (c == '\033') {
                inEscape = true;
                continue;
            }
            if (inEscape) {
                if (c == 'm') {
                    inEscape = false;
                }
                continue;
            }
            if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
                ++width;
            }
        }
        return width;
    }

    void renderHudCards(std::string& frame, const GameView& view, std::uint32_t delayMs, int cols, bool truecolor,
        PaletteId paletteId) noexcept
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

        const auto& theme = ColorPalette::get(paletteId);
        const auto ansi = getThemeAnsi(paletteId);
        const std::string borderCol = truecolor ? appendTruecolorStr(theme.hudBorder) : ansi.border;
        const std::string resetCol = "\033[0m";

        const std::array<std::string, 6> labels = {"SCORE", "HIGH", "LIVES", "LEVEL", "TIME", "STATUS"};
        const std::array<std::string, 6> labelCols = {truecolor ? appendTruecolorStr(theme.textLabel) : "\033[1;33m",
            truecolor ? appendTruecolorStr(theme.textLabel) : "\033[1;33m",
            truecolor ? appendTruecolorStr(theme.markerDiamond) : "\033[1;31m",
            truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;35m",
            truecolor ? appendTruecolorStr(theme.progressBarTarget) : "\033[1;32m",
            truecolor ? appendTruecolorStr(theme.textAccent) : "\033[1;36m"};

        // Prepare Values
        const std::string scoreVal = std::to_string(view.stats.score);
        const std::string scoreCol = truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;37m";

        const std::string highVal = std::to_string(view.stats.highScore);
        const std::string highCol = truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;33m";

        std::string livesVal;
        if (view.stats.lives <= 4) {
            for (std::uint16_t l {0}; l < view.stats.lives; ++l) {
                livesVal += "\xe2\x99\xa5"; // ♥
            }
            livesVal += " (" + std::to_string(view.stats.lives) + ")";
        } else {
            livesVal = "\xe2\x99\xa5 x" + std::to_string(view.stats.lives);
        }
        const std::string livesCol = truecolor ? appendTruecolorStr(theme.markerDiamond) : "\033[1;31m";

        std::string lvlVal = std::to_string(view.stats.level);
        if (view.stats.multiplier > 1) {
            lvlVal += " [x" + std::to_string(view.stats.multiplier) + "]";
        }
        const std::string lvlCol = truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;35m";

        const auto secondsRemaining = (view.stats.timeRemainingMs + 999U) / 1000U;
        const std::string timeVal = std::to_string(secondsRemaining) + "s";
        std::string timeCol = truecolor ? appendTruecolorStr(theme.progressBarTarget) : "\033[1;32m";
        if (view.stats.timeUp || secondsRemaining <= 10U) {
            timeCol = truecolor ? appendTruecolorStr(theme.markerDiamond) : "\033[1;31m";
        } else if (secondsRemaining <= 20U) {
            timeCol = truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;33m";
        }

        std::string statusVal = "READY";
        std::string statusCol = truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;33m";
        if (view.state == GameState::Attract) {
            statusVal = "DEMO";
            statusCol = truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;33m";
        } else if (view.state == GameState::Playing) {
            statusVal = "PLAYING";
            statusCol = truecolor ? appendTruecolorStr(theme.progressBarTarget) : "\033[1;32m";
        } else if (view.state == GameState::LevelComplete) {
            if (view.stats.spiralBonus) {
                statusVal = "SPIRAL!";
            } else if (view.stats.qixTrapped) {
                statusVal = "TRAPPED!";
            } else if (view.stats.splitBonus) {
                statusVal = "SPLIT!";
            } else {
                statusVal = "CLEARED";
            }
            statusCol = truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;33m";
        } else if (view.state == GameState::NameEntry) {
            statusVal = "INITIALS";
            statusCol = truecolor ? appendTruecolorStr(theme.textAccent) : "\033[1;36m";
        } else if (view.state == GameState::HallOfFame) {
            statusVal = "HOF";
            statusCol = truecolor ? appendTruecolorStr(theme.textAccent) : "\033[1;36m";
        } else if (view.state == GameState::GameOver) {
            statusVal = "GAMEOVER";
            statusCol = truecolor ? appendTruecolorStr(theme.markerDiamond) : "\033[1;31m";
        }

        const std::array<std::string, 6> vals = {scoreVal, highVal, livesVal, lvlVal, timeVal, statusVal};
        const std::array<std::string, 6> valCols = {scoreCol, highCol, livesCol, lvlCol, timeCol, statusCol};

        // Compute 6 card column widths inside interior 'cols'
        constexpr int numCards = 6;
        constexpr int totalDividers = numCards - 1; // 5 internal dividers
        const int availCols = std::max(numCards * 6, cols - totalDividers);
        std::array<int, numCards> cardWidths {};
        const int baseW = availCols / numCards;
        const int extraW = availCols % numCards;
        for (int i {0}; i < numCards; ++i) {
            cardWidths[static_cast<std::size_t>(i)] = baseW + (i < extraW ? 1 : 0);
        }

        // --- Row 1: Top Border with Card Headers ---
        frame += borderCol + "┌";
        for (int i {0}; i < numCards; ++i) {
            if (i > 0) {
                frame += borderCol + "┬";
            }
            const int w = cardWidths[static_cast<std::size_t>(i)];
            const auto& label = labels[static_cast<std::size_t>(i)];
            const auto& lCol = labelCols[static_cast<std::size_t>(i)];

            std::string badge;
            int badgeLen = 0;
            if (w >= static_cast<int>(label.size()) + 4) {
                badge = "[" + label + "]";
                badgeLen = static_cast<int>(label.size()) + 2;
            } else {
                badge = label;
                badgeLen = static_cast<int>(label.size());
            }

            const int padTotal = std::max(0, w - badgeLen);
            const int padL = padTotal / 2;
            const int padR = padTotal - padL;

            for (int p {0}; p < padL; ++p) {
                frame += "─";
            }
            frame += lCol + badge + borderCol;
            for (int p {0}; p < padR; ++p) {
                frame += "─";
            }
        }
        frame += borderCol + "┐" + resetCol + "\n";

        // --- Row 2: Card Values Row ---
        frame += borderCol + "│";
        for (int i {0}; i < numCards; ++i) {
            if (i > 0) {
                frame += borderCol + "│";
            }
            const int w = cardWidths[static_cast<std::size_t>(i)];
            const auto& val = vals[static_cast<std::size_t>(i)];
            const auto& vCol = valCols[static_cast<std::size_t>(i)];
            const int vLen = visibleWidth(val);

            const int padTotal = std::max(0, w - vLen);
            const int padL = padTotal / 2;
            const int padR = padTotal - padL;

            for (int p {0}; p < padL; ++p) {
                frame += ' ';
            }
            frame += vCol + val + borderCol;
            for (int p {0}; p < padR; ++p) {
                frame += ' ';
            }
        }
        frame += borderCol + "│" + resetCol + "\n";

        // --- Row 3: Card Divider Row with ┴ junctions ---
        frame += borderCol + "├";
        for (int i {0}; i < numCards; ++i) {
            if (i > 0) {
                frame += borderCol + "┴";
            }
            const int w = cardWidths[static_cast<std::size_t>(i)];
            for (int p {0}; p < w; ++p) {
                frame += "─";
            }
        }
        frame += borderCol + "┤" + resetCol + "\n";

        // --- Row 4: Territory Progress Bar Card ---
        const double claimed = (view.playfield && view.playfield->getInteriorCount() > 0)
            ? (static_cast<double>(view.playfield->getClaimedCount()) * 100.0
                / static_cast<double>(view.playfield->getInteriorCount()))
            : static_cast<double>(view.stats.claimedPercent);
        const std::uint16_t target = view.stats.targetPercent;

        const std::string barLabel = (cols >= 60) ? " Territory: [" : ((cols >= 45) ? " Claim: [" : " [");
        const int barLabelLen = static_cast<int>(barLabel.size());

        char pctBuf[80];
        if (claimed < static_cast<double>(target)) {
            if (cols >= 50) {
                std::snprintf(pctBuf, sizeof(pctBuf), "%4.1f%% / %u%% Target", claimed, target);
            } else {
                std::snprintf(pctBuf, sizeof(pctBuf), "%4.1f%% / %u%%", claimed, target);
            }
        } else {
            const int bonus = static_cast<int>(claimed) - static_cast<int>(target);
            if (cols >= 60) {
                std::snprintf(pctBuf, sizeof(pctBuf), "%4.1f%% / %u%% (MET! +%d%% Bonus)", claimed, target, bonus);
            } else {
                std::snprintf(pctBuf, sizeof(pctBuf), "%4.1f%% (+%d%%)", claimed, bonus);
            }
        }
        const std::string pctStr(pctBuf);
        const int pctLen = static_cast<int>(pctStr.size());

        const std::string delayStr = "Delay: " + std::to_string(delayMs) + "ms";
        const int delayLen = static_cast<int>(delayStr.size());

        const int fixedNonBar = barLabelLen + 2 + pctLen + 1;
        bool includeDelay = false;
        if (cols >= fixedNonBar + delayLen + 16) {
            includeDelay = true;
        }
        const int barBudget = cols - fixedNonBar - (includeDelay ? (delayLen + 2) : 0);
        const int barWidth = std::clamp(barBudget, 8, 50);

        const int totalEighths = static_cast<int>(std::round((claimed / 100.0) * barWidth * 8.0));
        const int fullChars = totalEighths / 8;
        const int fracIdx = totalEighths % 8;
        const int targetChar = static_cast<int>(std::round((static_cast<double>(target) / 100.0) * barWidth));

        const std::string fillCol = truecolor
            ? ((claimed < static_cast<double>(target)) ? appendTruecolorStr(theme.progressBarFill)
                                                       : appendTruecolorStr(theme.progressBarTarget))
            : ((claimed < static_cast<double>(target)) ? "\033[1;36m" : "\033[1;33m");
        const std::string emptyCol = truecolor ? appendTruecolorStr(theme.progressBarBg) : "\033[2;37m";
        const std::string targetCol = truecolor ? appendTruecolorStr(theme.markerDiamond) : "\033[1;31m";

        frame += borderCol + "│" + resetCol + barLabel;
        for (int i {0}; i < barWidth; ++i) {
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
        frame += resetCol + "] ";

        if (claimed < static_cast<double>(target)) {
            frame += (truecolor ? appendTruecolorStr(theme.progressBarFill) : "\033[1;36m") + pctStr + resetCol;
        } else {
            frame += (truecolor ? appendTruecolorStr(theme.progressBarTarget) : "\033[1;33m") + pctStr + resetCol;
        }

        const int currentVisible = barLabelLen + barWidth + 2 + pctLen;
        const int targetWidth = cols - (includeDelay ? (delayLen + 1) : 0);
        const int padSpaces = std::max(0, targetWidth - currentVisible);
        for (int p {0}; p < padSpaces; ++p) {
            frame += ' ';
        }

        if (includeDelay) {
            frame += (truecolor ? "\033[2;38;2;120;140;180m" : "\033[2;37m") + delayStr + resetCol + " ";
        }

        frame += borderCol + "│" + resetCol + "\n";

        // --- Row 5: Bottom Border of HUD Cards Box ---
        frame += borderCol + "└";
        for (int i {0}; i < cols; ++i) {
            frame += "─";
        }
        frame += borderCol + "┘" + resetCol + "\n";
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
    m_prevLines.clear();
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

    m_prevLines.clear();
    m_initialized = false;
}

void TuiRenderer::clearScreen() noexcept
{
    m_prevLines.clear();
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
    // Vertical overhead: HUD Cards (5) + top border (1) + bottom border (1) + controls (1) + margin (1) = 9 lines
    const int charRows = std::max(10, term.rows - 9);
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
    if (m_brailleMode != enabled) {
        m_brailleMode = enabled;
        invalidateScreen();
    }
}

bool TuiRenderer::isBrailleMode() const noexcept
{
    return m_brailleMode;
}

void TuiRenderer::toggleBrailleMode() noexcept
{
    m_brailleMode = !m_brailleMode;
    invalidateScreen();
}

void TuiRenderer::setTruecolor(bool enabled) noexcept
{
    if (m_truecolor != enabled) {
        m_truecolor = enabled;
        invalidateScreen();
    }
}

bool TuiRenderer::isTruecolor() const noexcept
{
    return m_truecolor;
}

void TuiRenderer::toggleTruecolor() noexcept
{
    m_truecolor = !m_truecolor;
    invalidateScreen();
}

void TuiRenderer::setPalette(PaletteId id) noexcept
{
    if (m_paletteId != id) {
        m_paletteId = id;
        invalidateScreen();
    }
}

PaletteId TuiRenderer::getPalette() const noexcept
{
    return m_paletteId;
}

void TuiRenderer::cyclePalette() noexcept
{
    setPalette(ColorPalette::next(m_paletteId));
}

void TuiRenderer::setArtEnabled(bool enabled) noexcept
{
    if (m_artEnabled != enabled) {
        m_artEnabled = enabled;
        invalidateScreen();
    }
}

bool TuiRenderer::isArtEnabled() const noexcept
{
    return m_artEnabled;
}

void TuiRenderer::toggleArt() noexcept
{
    m_artEnabled = !m_artEnabled;
    invalidateScreen();
}

void TuiRenderer::setArtScene(int scene) noexcept
{
    if (m_customArtScene != scene) {
        m_customArtScene = scene;
        invalidateScreen();
    }
}

void TuiRenderer::setDifferentialUpdates(bool enabled) noexcept
{
    m_differentialUpdates = enabled;
    if (!enabled) {
        invalidateScreen();
    }
}

bool TuiRenderer::isDifferentialUpdates() const noexcept
{
    return m_differentialUpdates;
}

void TuiRenderer::invalidateScreen() noexcept
{
    m_prevLines.clear();
}

char TuiRenderer::getTypedChar() const noexcept
{
    return m_typedChar;
}

void TuiRenderer::presentFrame(const std::string& frame) noexcept
{
    if (!m_differentialUpdates) {
        std::cout << frame << std::flush;
        return;
    }

    std::size_t startPos = 0;
    if (frame.rfind("\033[H", 0) == 0) {
        startPos = 3;
    }

    std::vector<std::string> currentLines;
    currentLines.reserve(32);

    std::size_t lineStart = startPos;
    while (lineStart < frame.size()) {
        const std::size_t lineEnd = frame.find('\n', lineStart);
        if (lineEnd == std::string::npos) {
            currentLines.push_back(frame.substr(lineStart));
            break;
        }
        currentLines.push_back(frame.substr(lineStart, lineEnd - lineStart));
        lineStart = lineEnd + 1;
    }

    if (!currentLines.empty() && currentLines.back().empty() && lineStart > startPos && frame.back() == '\n') {
        currentLines.pop_back();
    }

    std::string diffOutput;
    diffOutput.reserve(4096);

    const std::size_t curSize = currentLines.size();
    const std::size_t prevSize = m_prevLines.size();

    for (std::size_t r = 0; r < curSize; ++r) {
        if (r >= prevSize || currentLines[r] != m_prevLines[r]) {
            diffOutput += "\033[" + std::to_string(r + 1) + ";1H" + currentLines[r] + "\033[K\033[0m";
        }
    }

    for (std::size_t r = curSize; r < prevSize; ++r) {
        diffOutput += "\033[" + std::to_string(r + 1) + ";1H\033[K\033[0m";
    }

    if (!diffOutput.empty()) {
        diffOutput += "\033[H";
        std::cout << diffOutput << std::flush;
    }

    m_prevLines = std::move(currentLines);
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

    const auto playfieldWidth = view.playfield->getWidth();
    const auto term = queryTerminalSize();
    const int maxCols = std::max(20, term.cols - 4);
    const std::int32_t stepX = std::max(1, (playfieldWidth + maxCols - 1) / maxCols);
    const std::int32_t cols = m_brailleMode ? ((playfieldWidth + 1) / 2) : ((playfieldWidth + stepX - 1) / stepX);

    // 1. Modern Arcade HUD Cards Deck
    renderHudCards(frame, view, delayMs, cols, m_truecolor, m_paletteId);

    if (view.state == GameState::Attract) {
        if (view.attractStage == AttractStage::TitleScores) {
            renderHallOfFame(frame, view.highScoreTable, false, true);
            presentFrame(frame);
            return;
        } else if (view.attractStage == AttractStage::Instructions) {
            renderInstructions(frame);
            presentFrame(frame);
            return;
        }
    }

    if (view.state == GameState::NameEntry) {
        renderNameEntry(frame, view.nameEntry, view.stats);
        presentFrame(frame);
        return;
    }
    if (view.state == GameState::HallOfFame || view.state == GameState::GameOver) {
        renderHallOfFame(frame, view.highScoreTable, view.state == GameState::GameOver, false);
        presentFrame(frame);
        return;
    }

    // 2. Playfield Grid (Braille Sub-Pixel or Classic ASCII)
    if (m_brailleMode) {
        renderBraillePlayfield(frame, view);
    } else {
        renderAsciiPlayfield(frame, view);
    }

    // 3. Controls Legend
    if (view.state == GameState::LevelComplete) {
        const auto artScene = (m_customArtScene >= 0) ? BackgroundArt::fromIndex(m_customArtScene)
                                                      : BackgroundArt::getSceneForLevel(view.stats.level);
        if (m_artEnabled) {
            frame += "  \033[1;33m*** ART UNMASKED: " + std::string(BackgroundArt::getSceneName(artScene))
                + " ***   \033[1;32mPRESS [SPACE/ENTER] FOR NEXT LEVEL\033[0m\n";
        } else {
            frame += "  \033[1;32m*** LEVEL COMPLETE ***   PRESS [SPACE/ENTER] FOR NEXT LEVEL\033[0m\n";
        }
    } else if (view.state == GameState::Attract) {
        frame += "  \033[1;33m*** ARCADE DEMO MODE ***   \033[1;32mINSERT COIN - PRESS ANY KEY TO PLAY\033[0m\n";
    } else if (m_lastTermSize.cols >= 115) {
        frame += "\033[2mControls: [WASD/Arrows] Move | [Space] Slow | [F] Fast | [V/F5] Art | [X] Border | [P/F4] "
                 "Theme | [B] "
                 "Braille/ASCII | [T] RGB | [-/+] Speed | [R] Reset | [Q] Quit\033[0m\n";
    } else {
        frame += "\033[2mControls: [WASD] Move | [Space/F] Draw | [V] Art | [P/F4] Theme | [B] Mode | [T] RGB | "
                 "[R] Reset | [Q] Quit\033[0m\n";
    }

    presentFrame(frame);
}

void TuiRenderer::renderBraillePlayfield(std::string& frame, const GameView& view) noexcept
{
    const auto width = view.playfield->getWidth();
    const auto height = view.playfield->getHeight();

    const std::int32_t cols = (width + 1) / 2;
    const std::int32_t rows = (height + 3) / 4;

    std::vector<BrailleCell> grid(static_cast<std::size_t>(cols * rows));
    const auto& theme = ColorPalette::get(m_paletteId);
    const auto ansiColors = getThemeAnsi(m_paletteId);
    const auto artScene = (m_customArtScene >= 0) ? BackgroundArt::fromIndex(m_customArtScene)
                                                  : BackgroundArt::getSceneForLevel(view.stats.level);

    // 1. Plot Playfield Cells (Borders, Claimed Areas, Active Stix)
    for (std::int32_t y {0}; y < height; ++y) {
        for (std::int32_t x {0}; x < width; ++x) {
            const auto state = view.playfield->getCell(x, y);
            const int cx = x / 2;
            const int cy = y / 4;
            auto& cell = grid[static_cast<std::size_t>(cy * cols + cx)];
            const std::uint8_t dot = kDotMask[y % 4][x % 2];

            if (view.state == GameState::LevelComplete && m_artEnabled) {
                cell.dots |= dot;
                if (cell.priority < 1) {
                    cell.priority = 1;
                    if (m_truecolor) {
                        cell.isRgb = true;
                        const float u = static_cast<float>(static_cast<double>(x) + 0.5) / static_cast<float>(width);
                        const float v = static_cast<float>(static_cast<double>(y) + 0.5) / static_cast<float>(height);
                        const auto artPixel = BackgroundArt::samplePixel(artScene, u, v);
                        cell.r = artPixel.r;
                        cell.g = artPixel.g;
                        cell.b = artPixel.b;
                    } else {
                        cell.isRgb = false;
                        cell.ansiColor = ansiColors.claimedSlow;
                    }
                }
            } else if (state == CellState::Empty) {
                continue;
            } else {
                cell.dots |= dot;
            }

            if (state == CellState::Border) {
                if (cell.priority < 2) {
                    cell.priority = 2;
                    if (m_truecolor) {
                        cell.isRgb = true;
                        cell.r = theme.playfieldBorder.r;
                        cell.g = theme.playfieldBorder.g;
                        cell.b = theme.playfieldBorder.b;
                    } else {
                        cell.isRgb = false;
                        cell.ansiColor = ansiColors.border;
                    }
                }
            } else if (state == CellState::ActiveStix) {
                if (cell.priority < 3) {
                    cell.priority = 3;
                    if (m_truecolor) {
                        cell.isRgb = true;
                        cell.r = theme.activeStix.r;
                        cell.g = theme.activeStix.g;
                        cell.b = theme.activeStix.b;
                    } else {
                        cell.isRgb = false;
                        cell.ansiColor = ansiColors.activeStix;
                    }
                }
            } else if (state == CellState::ClaimedSlow) {
                if (cell.priority < 1) {
                    cell.priority = 1;
                    if (m_truecolor) {
                        cell.isRgb = true;
                        if (m_artEnabled) {
                            const float u
                                = static_cast<float>(static_cast<double>(x) + 0.5) / static_cast<float>(width);
                            const float v
                                = static_cast<float>(static_cast<double>(y) + 0.5) / static_cast<float>(height);
                            const auto artPixel = BackgroundArt::samplePixel(artScene, u, v);
                            cell.r = artPixel.r;
                            cell.g = artPixel.g;
                            cell.b = artPixel.b;
                        } else {
                            cell.r = theme.claimedSlow.r;
                            cell.g = theme.claimedSlow.g;
                            cell.b = theme.claimedSlow.b;
                        }
                    } else {
                        cell.isRgb = false;
                        cell.ansiColor = ansiColors.claimedSlow;
                    }
                }
            } else if (state == CellState::ClaimedFast) {
                if (cell.priority < 1) {
                    cell.priority = 1;
                    if (m_truecolor) {
                        cell.isRgb = true;
                        if (m_artEnabled) {
                            const float u
                                = static_cast<float>(static_cast<double>(x) + 0.5) / static_cast<float>(width);
                            const float v
                                = static_cast<float>(static_cast<double>(y) + 0.5) / static_cast<float>(height);
                            const auto artPixel = BackgroundArt::samplePixel(artScene, u, v);
                            cell.r = static_cast<std::uint8_t>(artPixel.r / 3);
                            cell.g = static_cast<std::uint8_t>(std::min(255, artPixel.g * 2 / 3 + 30));
                            cell.b = static_cast<std::uint8_t>(std::min(255, artPixel.b * 4 / 5 + 60));
                        } else {
                            cell.r = theme.claimedFast.r;
                            cell.g = theme.claimedFast.g;
                            cell.b = theme.claimedFast.b;
                        }
                    } else {
                        cell.isRgb = false;
                        cell.ansiColor = ansiColors.claimedFast;
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
                    cell.r = theme.activeStix.r;
                    cell.g = theme.activeStix.g;
                    cell.b = theme.activeStix.b;
                } else {
                    cell.isRgb = false;
                    cell.ansiColor = ansiColors.activeStix;
                }
            }
        }
    }

    // 3. Qix Ribbons (Sub-Pixel Vector Bresenham Line Rasterization with 24-bit Truecolor Cycling)
    for (const auto& ribbon : view.qixRibbons) {
        const auto totalSegs = ribbon.size();
        for (std::size_t segIdx {0}; segIdx < totalSegs; ++segIdx) {
            const auto& seg = ribbon[segIdx];
            const auto ribbonCol = ColorPalette::computeRibbonColor(theme, m_colorCycle, segIdx, totalSegs);
            const Rgb segRgb {ribbonCol.r, ribbonCol.g, ribbonCol.b};
            const char* segAnsi = "\033[1;31m";
            if (theme.ribbonMode == RibbonColorMode::NeonGradient) {
                segAnsi = (segIdx % 2 == 0) ? "\033[1;35m" : "\033[1;36m";
            } else if (theme.ribbonMode == RibbonColorMode::MonochromeAmber) {
                segAnsi = (ribbonCol.r > 150) ? "\033[1;33m" : "\033[0;33m";
            } else if (theme.ribbonMode == RibbonColorMode::MonochromeGreen) {
                segAnsi = (ribbonCol.g > 150) ? "\033[1;32m" : "\033[0;32m";
            } else {
                segAnsi = (segIdx == 0) ? "\033[1;31m" : ((segIdx < 3) ? "\033[1;35m" : "\033[0;35m");
            }

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
                        cell.r = theme.superSparx.r;
                        cell.g = theme.superSparx.g;
                        cell.b = theme.superSparx.b;
                    } else {
                        cell.r = theme.sparx.r;
                        cell.g = theme.sparx.g;
                        cell.b = theme.sparx.b;
                    }
                } else {
                    cell.isRgb = false;
                    cell.ansiColor = sp.isSuper ? ansiColors.superSparx : ansiColors.sparx;
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
                    cell.r = theme.sparx.r;
                    cell.g = theme.sparx.g;
                    cell.b = theme.sparx.b;
                } else {
                    cell.isRgb = false;
                    cell.ansiColor = ansiColors.sparx;
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
                cell.r = theme.fuse.r;
                cell.g = theme.fuse.g;
                cell.b = theme.fuse.b;
            } else {
                cell.isRgb = false;
                cell.ansiColor = ansiColors.fuse;
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
            if (view.drawMode != DrawMode::None) {
                cell.r = theme.textValue.r;
                cell.g = theme.textValue.g;
                cell.b = theme.textValue.b;
            } else {
                cell.r = theme.marker.r;
                cell.g = theme.marker.g;
                cell.b = theme.marker.b;
            }
        } else {
            cell.isRgb = false;
            cell.ansiColor = ansiColors.marker;
        }
    }

    // 7. Output Rendered Braille Frame with Border Framing
    const std::string borderCol = m_truecolor ? appendTruecolorStr(theme.hudBorder) : ansiColors.border;
    const std::string titleCol = m_truecolor ? appendTruecolorStr(theme.textAccent) : "\033[1;36m";
    std::string modeBadge = m_brailleMode ? "BRAILLE" : "ASCII";
    if (m_truecolor) {
        modeBadge += " RGB";
    }
    const std::string titleBadge = std::string(" QIX · ") + modeBadge + " · " + theme.name + " ";
    const int badgeLen = static_cast<int>(titleBadge.size());

    if (cols >= badgeLen + 6) {
        frame += borderCol + "┌─[" + titleCol + titleBadge + borderCol + "]";
        const int remain = cols - (badgeLen + 4);
        for (int cx {0}; cx < remain; ++cx) {
            frame += "─";
        }
        frame += "┐\033[0m\n";
    } else {
        frame += borderCol + "┌";
        for (int cx {0}; cx < cols; ++cx) {
            frame += "─";
        }
        frame += "┐\033[0m\n";
    }

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
    const int maxRows = std::max(10, term.rows - 9);

    const std::int32_t stepX = std::max(1, (width + maxCols - 1) / maxCols);
    const std::int32_t stepY = std::max(1, (height + maxRows - 1) / maxRows);
    const std::int32_t cols = (width + stepX - 1) / stepX;

    const auto& theme = ColorPalette::get(m_paletteId);
    const auto ansi = getThemeAnsi(m_paletteId);
    const auto artScene = (m_customArtScene >= 0) ? BackgroundArt::fromIndex(m_customArtScene)
                                                  : BackgroundArt::getSceneForLevel(view.stats.level);
    const std::string borderCol = m_truecolor ? appendTruecolorStr(theme.hudBorder) : ansi.border;
    const std::string titleCol = m_truecolor ? appendTruecolorStr(theme.textAccent) : "\033[1;36m";
    const std::string titleBadge = std::string(" QIX · ASCII · ") + theme.name + " ";
    const int badgeLen = static_cast<int>(titleBadge.size());

    // Top border
    if (cols >= badgeLen + 6) {
        frame += borderCol + "┌─[" + titleCol + titleBadge + borderCol + "]";
        const int remain = cols - (badgeLen + 4);
        for (std::int32_t cx {0}; cx < remain; ++cx) {
            frame += "─";
        }
        frame += "┐\033[0m\n";
    } else {
        frame += borderCol + "┌";
        for (std::int32_t cx {0}; cx < cols; ++cx) {
            frame += "─";
        }
        frame += "┐\033[0m\n";
    }

    for (std::int32_t y {0}; y < height; y += stepY) {
        frame += borderCol + "│\033[0m";
        for (std::int32_t x {0}; x < width; x += stepX) {
            Point p {x, y};

            // Player Marker
            if (p == view.markerPos) {
                if (m_truecolor) {
                    appendTruecolor(frame, theme.marker.r, theme.marker.g, theme.marker.b);
                    frame += "@\033[0m";
                } else {
                    frame += ansi.marker;
                    frame += "@\033[0m";
                }
                continue;
            }

            // Fuse
            if (view.fusePos.has_value() && p == view.fusePos.value()) {
                if (m_truecolor) {
                    appendTruecolor(frame, theme.fuse.r, theme.fuse.g, theme.fuse.b);
                    frame += "!\033[0m";
                } else {
                    frame += ansi.fuse;
                    frame += "!\033[0m";
                }
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
                if (m_truecolor) {
                    const auto& sc = isSuper ? theme.superSparx : theme.sparx;
                    appendTruecolor(frame, sc.r, sc.g, sc.b);
                    frame += (isSuper ? "S\033[0m" : "$\033[0m");
                } else {
                    frame += (isSuper ? ansi.superSparx : ansi.sparx);
                    frame += (isSuper ? "S\033[0m" : "$\033[0m");
                }
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
                        const auto ribbonCol = ColorPalette::computeRibbonColor(theme, m_colorCycle, segIdx, totalSegs);
                        qixRgb = Rgb {ribbonCol.r, ribbonCol.g, ribbonCol.b};
                        if (theme.ribbonMode == RibbonColorMode::NeonGradient) {
                            qixAnsi = (segIdx % 2 == 0) ? "\033[1;35m" : "\033[1;36m";
                        } else if (theme.ribbonMode == RibbonColorMode::MonochromeAmber) {
                            qixAnsi = (ribbonCol.r > 150) ? "\033[1;33m" : "\033[0;33m";
                        } else if (theme.ribbonMode == RibbonColorMode::MonochromeGreen) {
                            qixAnsi = (ribbonCol.g > 150) ? "\033[1;32m" : "\033[0;32m";
                        } else {
                            qixAnsi = (segIdx == 0) ? "\033[1;31m" : ((segIdx < 3) ? "\033[1;35m" : "\033[0;35m");
                        }
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

            // Stix trail & Playfield Cells
            const auto state = view.playfield->getCell(x, y);
            if (state == CellState::ActiveStix) {
                if (m_truecolor) {
                    appendTruecolor(frame, theme.activeStix.r, theme.activeStix.g, theme.activeStix.b);
                    frame += "*\033[0m";
                } else {
                    frame += ansi.activeStix;
                    frame += "*\033[0m";
                }
            } else if (state == CellState::Border) {
                if (m_truecolor) {
                    appendTruecolor(frame, theme.playfieldBorder.r, theme.playfieldBorder.g, theme.playfieldBorder.b);
                    frame += "#\033[0m";
                } else {
                    frame += ansi.border;
                    frame += "#\033[0m";
                }
            } else if (view.state == GameState::LevelComplete && m_artEnabled && state == CellState::Empty) {
                if (m_truecolor) {
                    const float u = static_cast<float>(static_cast<double>(x) + 0.5) / static_cast<float>(width);
                    const float v = static_cast<float>(static_cast<double>(y) + 0.5) / static_cast<float>(height);
                    const auto artPixel = BackgroundArt::samplePixel(artScene, u, v);
                    appendTruecolor(frame, artPixel.r, artPixel.g, artPixel.b);
                    frame += "\xe2\x96\x88\033[0m"; // solid block █
                } else {
                    frame += ansi.claimedSlow;
                    frame += ".\033[0m";
                }
            } else if (state == CellState::ClaimedSlow) {
                if (m_truecolor) {
                    if (m_artEnabled) {
                        const float u = static_cast<float>(static_cast<double>(x) + 0.5) / static_cast<float>(width);
                        const float v = static_cast<float>(static_cast<double>(y) + 0.5) / static_cast<float>(height);
                        const auto artPixel = BackgroundArt::samplePixel(artScene, u, v);
                        appendTruecolor(frame, artPixel.r, artPixel.g, artPixel.b);
                        frame += "\xe2\x96\x88\033[0m";
                    } else {
                        appendTruecolor(frame, theme.claimedSlow.r, theme.claimedSlow.g, theme.claimedSlow.b);
                        frame += ".\033[0m";
                    }
                } else {
                    frame += ansi.claimedSlow;
                    frame += ".\033[0m";
                }
            } else if (state == CellState::ClaimedFast) {
                if (m_truecolor) {
                    if (m_artEnabled) {
                        const float u = static_cast<float>(static_cast<double>(x) + 0.5) / static_cast<float>(width);
                        const float v = static_cast<float>(static_cast<double>(y) + 0.5) / static_cast<float>(height);
                        const auto artPixel = BackgroundArt::samplePixel(artScene, u, v);
                        const auto r = static_cast<std::uint8_t>(artPixel.r / 3);
                        const auto g = static_cast<std::uint8_t>(std::min(255, artPixel.g * 2 / 3 + 30));
                        const auto b = static_cast<std::uint8_t>(std::min(255, artPixel.b * 4 / 5 + 60));
                        appendTruecolor(frame, r, g, b);
                        frame += "\xe2\x96\x92\033[0m"; // medium shade ▒
                    } else {
                        appendTruecolor(frame, theme.claimedFast.r, theme.claimedFast.g, theme.claimedFast.b);
                        frame += ",\033[0m";
                    }
                } else {
                    frame += ansi.claimedFast;
                    frame += ",\033[0m";
                }
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

PlayerCommand TuiRenderer::processInput(std::string_view bytes, TuiAction& action) noexcept
{
    PlayerCommand cmd {};
    action = TuiAction::None;
    m_typedChar = 0;

    if (!bytes.empty()) {
        m_inputQueue.append(bytes);
    }

    std::size_t i {0};
    while (i < m_inputQueue.size()) {
        const unsigned char ch = static_cast<unsigned char>(m_inputQueue[i]);
        if (ch == '\033') {
            // Check for incomplete escape sequence at end of buffer
            if (i + 1 >= m_inputQueue.size()) {
                // Lone ESC at end of buffer; hold for next chunk
                break;
            }
            if (m_inputQueue[i + 1] == '[') {
                // CSI sequence: \033[ ...
                std::size_t j {i + 2};
                // Parameter bytes (0x30..0x3F) and intermediate bytes (0x20..0x2F)
                while (j < m_inputQueue.size() && static_cast<unsigned char>(m_inputQueue[j]) >= 0x20
                    && static_cast<unsigned char>(m_inputQueue[j]) <= 0x3F) {
                    ++j;
                }
                if (j >= m_inputQueue.size()) {
                    // Incomplete CSI sequence; hold for next chunk
                    break;
                }
                const char finalChar = m_inputQueue[j];
                switch (finalChar) {
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
                case '~': {
                    const auto param = m_inputQueue.substr(i + 2, j - (i + 2));
                    if (param == "14" || param == "11" || param == "1;4P" || param == "1;*P") {
                        action = TuiAction::CyclePalette;
                    } else if (param == "15" || param == "1;5P" || param == "1;*Q") {
                        action = TuiAction::ToggleArt;
                    }
                    break;
                }
                default:
                    break;
                }
                i = j + 1;
                continue;
            }
            if (m_inputQueue[i + 1] == 'O') {
                // SS3 sequence: \033O ...
                if (i + 2 >= m_inputQueue.size()) {
                    // Incomplete SS3; hold for next chunk
                    break;
                }
                switch (m_inputQueue[i + 2]) {
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
                case 'S':
                    action = TuiAction::CyclePalette;
                    break;
                default:
                    break;
                }
                i += 3;
                continue;
            }

            // Lone ESC or unrecognized escape prefix; advance past ESC
            ++i;
            continue;
        }

        // Regular character processing
        m_typedChar = static_cast<char>(ch);
        switch (ch) {
        case 8:
        case 127:
            action = TuiAction::Backspace;
            break;
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
            action = TuiAction::SpeedDown;
            break;
        case '+':
        case '=':
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
            action = TuiAction::Confirm;
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
        case 'p':
        case 'P':
            action = TuiAction::CyclePalette;
            break;
        case 'v':
        case 'V':
            action = TuiAction::ToggleArt;
            break;
        case '\n':
        case '\r':
            action = TuiAction::Confirm;
            break;
        default:
            if (std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '!' || ch == '.' || ch == '?') {
                action = TuiAction::CharInput;
            }
            break;
        }
        ++i;
    }

    if (i > 0) {
        m_inputQueue.erase(0, i);
    }
    // Discard buffer if malformed noise accumulates beyond limit
    if (m_inputQueue.size() > 32) {
        m_inputQueue.clear();
    }

    return cmd;
}

PlayerCommand TuiRenderer::pollInput(TuiAction& action) noexcept
{
    const bool resized = checkAndHandleResize();

    std::string readBuf {};
#ifdef _WIN32
    while (_kbhit()) {
        const int ch = _getch();
        if (ch == 0 || ch == 224) { // Extended key prefix
            const int ext = _getch();
            switch (ext) {
            case 72:
                readBuf += "\033[A";
                break;
            case 80:
                readBuf += "\033[B";
                break;
            case 75:
                readBuf += "\033[D";
                break;
            case 77:
                readBuf += "\033[C";
                break;
            case 62: // F4
                readBuf += "\033OS";
                break;
            case 63: // F5
                readBuf += "\033[15~";
                break;
            default:
                break;
            }
        } else {
            readBuf.push_back(static_cast<char>(ch));
        }
    }
#else
    char buf[128] {0};
    while (true) {
        const auto n = read(STDIN_FILENO, buf, sizeof(buf));
        if (n <= 0) {
            break;
        }
        readBuf.append(buf, static_cast<std::size_t>(n));
    }
#endif

    PlayerCommand cmd = processInput(readBuf, action);

    if (action == TuiAction::None && resized) {
        action = TuiAction::Resize;
    }

    return cmd;
}

static std::string formatScore(std::uint32_t score)
{
    std::string s = std::to_string(score);
    int n = static_cast<int>(s.length()) - 3;
    while (n > 0) {
        s.insert(static_cast<std::size_t>(n), ",");
        n -= 3;
    }
    return s;
}

static int visualLength(const std::string& str) noexcept
{
    int len = 0;
    std::size_t i = 0;
    while (i < str.size()) {
        if (str[i] == '\033') {
            while (i < str.size() && str[i] != 'm') {
                ++i;
            }
            if (i < str.size() && str[i] == 'm') {
                ++i;
            }
            continue;
        }
        const auto uc = static_cast<unsigned char>(str[i]);
        if (uc < 0x80) {
            ++len;
            ++i;
        } else if ((uc & 0xE0) == 0xC0) {
            ++len;
            i += 2;
        } else if ((uc & 0xF0) == 0xE0) {
            ++len;
            i += 3;
        } else if ((uc & 0xF8) == 0xF0) {
            len += 2;
            i += 4;
        } else {
            ++i;
        }
    }
    return len;
}

void TuiRenderer::renderNameEntry(std::string& frame, const NameEntryState& entry, const GameStats& stats) noexcept
{
    const auto& theme = ColorPalette::get(m_paletteId);
    const auto ansi = getThemeAnsi(m_paletteId);
    const std::string bCol = m_truecolor ? appendTruecolorStr(theme.hudBorder) : ansi.border;
    const std::string cCol = m_truecolor ? appendTruecolorStr(theme.textAccent) : "\033[1;36m";
    const std::string yCol = m_truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;33m";
    const std::string aBorder = m_truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;33m";
    const std::string inBorder = m_truecolor ? appendTruecolorStr(theme.hudBorder) : "\033[34m";
    const std::string inText = m_truecolor ? appendTruecolorStr(theme.textLabel) : "\033[1;37m";
    const std::string reset = "\033[0m";

    frame += "\n";
    frame += "  " + bCol + "┌────────────────────────────────────────────────────────────┐" + reset + "\n";
    frame += "  " + bCol + "│" + cCol + "                  ★ ARCADE HALL OF FAME ★                   " + bCol + "│"
        + reset + "\n";
    frame += "  " + bCol + "├────────────────────────────────────────────────────────────┤" + reset + "\n";

    // High Score Banner
    const std::string bannerText
        = "🏆 NEW HIGH SCORE RECORD! RANK #" + std::to_string(entry.rank) + " - SCORE: " + formatScore(stats.score);
    const int bVisLen = visualLength(bannerText);
    const int bPadL = std::max(0, (60 - bVisLen) / 2);
    const int bPadR = std::max(0, 60 - bVisLen - bPadL);
    frame += "  " + bCol + "│" + reset + std::string(bPadL, ' ') + yCol + bannerText + reset + std::string(bPadR, ' ')
        + bCol + "│" + reset + "\n";

    frame += "  " + bCol + "│                                                            │" + reset + "\n";
    frame += "  " + bCol + "│" + cCol + "                ENTER YOUR 3-LETTER INITIALS                " + bCol + "│"
        + reset + "\n";

    // Indicator Arrow Up Row
    const int activeSlot = std::clamp(static_cast<int>(entry.cursorIndex), 0, 2);
    const int arrowOffset = 19 + activeSlot * 10;
    frame += "  " + bCol + "│" + std::string(arrowOffset, ' ') + yCol + "▲" + reset
        + std::string(60 - arrowOffset - 1, ' ') + bCol + "│" + reset + "\n";

    // 3D Letter Cards Deck
    const bool blinkState = ((m_colorCycle / 4) % 2 == 0);
    const std::string padL(16, ' ');
    const std::string gap(3, ' ');
    const std::string padR(17, ' ');

    std::string cardTop[3];
    std::string cardMid[3];
    std::string cardBot[3];

    for (int i = 0; i < 3; ++i) {
        const char ch = (i < 3) ? entry.initials[i] : ' ';
        if (i == activeSlot) {
            cardTop[i] = aBorder + "╔═════╗" + reset;
            if (blinkState) {
                cardMid[i] = aBorder + "║" + (m_truecolor ? "\033[7;1;38;2;255;220;40m" : "\033[7;1;33m") + "  "
                    + std::string(1, ch) + "  " + reset + aBorder + "║" + reset;
            } else {
                cardMid[i] = aBorder + "║" + (m_truecolor ? "\033[1;38;2;255;255;255m" : "\033[1;37m") + "  "
                    + std::string(1, ch) + "  " + reset + aBorder + "║" + reset;
            }
            cardBot[i] = aBorder + "╚═════╝" + reset;
        } else {
            cardTop[i] = inBorder + "┌─────┐" + reset;
            cardMid[i]
                = inBorder + "│" + reset + inText + "  " + std::string(1, ch) + "  " + reset + inBorder + "│" + reset;
            cardBot[i] = inBorder + "└─────┘" + reset;
        }
    }

    frame += "  " + bCol + "│" + padL + cardTop[0] + gap + cardTop[1] + gap + cardTop[2] + padR + bCol + "│" + reset
        + "\n";
    frame += "  " + bCol + "│" + padL + cardMid[0] + gap + cardMid[1] + gap + cardMid[2] + padR + bCol + "│" + reset
        + "\n";
    frame += "  " + bCol + "│" + padL + cardBot[0] + gap + cardBot[1] + gap + cardBot[2] + padR + bCol + "│" + reset
        + "\n";

    // Indicator Arrow Down Row
    frame += "  " + bCol + "│" + std::string(arrowOffset, ' ') + yCol + "▼" + reset
        + std::string(60 - arrowOffset - 1, ' ') + bCol + "│" + reset + "\n";

    frame += "  " + bCol + "├────────────────────────────────────────────────────────────┤" + reset + "\n";

    // Control Legend
    const std::string legend = " [Arrows/WASD] Pick  [A-Z/0-9] Type  [Del] Back  [Enter] OK ";
    frame += "  " + bCol + "│" + cCol + legend + bCol + "│" + reset + "\n";
    frame += "  " + bCol + "└────────────────────────────────────────────────────────────┘" + reset + "\n";
}

void TuiRenderer::renderHallOfFame(
    std::string& frame, const HighScoreTable* table, bool isGameOver, bool isAttract) noexcept
{
    const auto& theme = ColorPalette::get(m_paletteId);
    const auto ansi = getThemeAnsi(m_paletteId);
    const std::string bCol = m_truecolor ? appendTruecolorStr(theme.hudBorder) : ansi.border;
    const std::string cCol = m_truecolor ? appendTruecolorStr(theme.textAccent) : "\033[1;36m";
    const std::string rCol = m_truecolor ? appendTruecolorStr(theme.markerDiamond) : "\033[1;31m";
    const std::string yCol = m_truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;33m";
    const std::string reset = "\033[0m";

    frame += "\n";
    if (isGameOver) {
        frame += "  " + rCol + "╔═════════════════════════ GAME OVER ═════════════════════════╗" + reset + "\n";
    } else if (isAttract) {
        frame += "  " + yCol + "╔════════════════════ TAITO 1981 - QIX ARCADE ════════════════════╗" + reset + "\n";
    }
    frame += "  " + bCol + "┌────────────────────────────────────────────────────────────┐" + reset + "\n";
    frame += "  " + bCol + "│" + cCol + "                  ★ ARCADE HALL OF FAME ★                   " + bCol + "│"
        + reset + "\n";
    frame += "  " + bCol + "├────────────────────────────────────────────────────────────┤" + reset + "\n";
    frame += "  " + bCol + "│" + cCol + "   RANK       NAME       SCORE        LEVEL       MODE      " + bCol + "│"
        + reset + "\n";
    frame += "  " + bCol + "├────────────────────────────────────────────────────────────┤" + reset + "\n";

    if (table != nullptr) {
        const auto& entries = table->getEntries();
        const std::size_t maxRows = std::min(entries.size(), static_cast<std::size_t>(8));

        for (std::size_t i {0}; i < maxRows; ++i) {
            const auto& e = entries[i];
            const char* modeStr = (e.mode == GameMode::Classic) ? "  CLASSIC  " : "  MODERN   ";

            std::string rankStr;
            std::string rowCol;
            if (i == 0) {
                rankStr = " 🥇 1ST  ";
                rowCol = m_truecolor ? "\033[1;38;2;255;215;0m" : "\033[1;33m";
            } else if (i == 1) {
                rankStr = " 🥈 2ND  ";
                rowCol = m_truecolor ? "\033[1;38;2;220;225;235m" : "\033[1;37m";
            } else if (i == 2) {
                rankStr = " 🥉 3RD  ";
                rowCol = m_truecolor ? "\033[1;38;2;205;127;50m" : "\033[33m";
            } else {
                rankStr = "    " + std::to_string(i + 1) + "TH  ";
                rowCol = m_truecolor ? "\033[38;2;120;200;230m" : "\033[36m";
            }

            char nameBuf[16];
            std::snprintf(nameBuf, sizeof(nameBuf), "  %-3s ", e.initials.c_str());

            char scoreBuf[32];
            const std::string sFormatted = formatScore(e.score);
            std::snprintf(scoreBuf, sizeof(scoreBuf), "%12s  ", sFormatted.c_str());

            char levelBuf[16];
            std::snprintf(levelBuf, sizeof(levelBuf), " LV %-2u  ", static_cast<unsigned>(e.level));

            frame += "  " + bCol + "│" + rowCol + "  " + rankStr + "  " + nameBuf + "  " + scoreBuf + "  " + levelBuf
                + "  " + modeStr + "  " + bCol + "│" + reset + "\n";
        }

        // Fill remaining rows if fewer than 8 entries
        for (std::size_t i {maxRows}; i < 8; ++i) {
            const std::string rankStr = "    " + std::to_string(i + 1) + "TH  ";
            const std::string dimCol = m_truecolor ? "\033[38;2;80;100;120m" : "\033[2;37m";
            frame += "  " + bCol + "│" + dimCol + "  " + rankStr + "    ---             0    LV  1    CLASSIC    "
                + bCol + "│" + reset + "\n";
        }
    }

    frame += "  " + bCol + "├────────────────────────────────────────────────────────────┤" + reset + "\n";
    const std::string prompt = isAttract ? "           INSERT COIN - PRESS [SPACE] TO PLAY            "
                                         : "              Press [R] or [Space] to Play Again            ";
    frame += "  " + bCol + "│" + (m_truecolor ? "\033[1;38;2;50;240;120m" : "\033[1;32m") + prompt + bCol + "│" + reset
        + "\n";
    frame += "  " + bCol + "└────────────────────────────────────────────────────────────┘" + reset + "\n";
}

void TuiRenderer::renderInstructions(std::string& frame) noexcept
{
    const auto& theme = ColorPalette::get(m_paletteId);
    const auto ansi = getThemeAnsi(m_paletteId);
    const std::string bCol = m_truecolor ? appendTruecolorStr(theme.hudBorder) : ansi.border;
    const std::string cCol = m_truecolor ? appendTruecolorStr(theme.textAccent) : "\033[1;36m";
    const std::string yCol = m_truecolor ? appendTruecolorStr(theme.textValue) : "\033[1;33m";
    const std::string gCol = m_truecolor ? "\033[1;38;2;50;240;120m" : "\033[1;32m";
    const std::string reset = "\033[0m";

    frame += "\n";
    frame += "  " + bCol + "┌────────────────────────────────────────────────────────────┐" + reset + "\n";
    frame += "  " + bCol + "│" + yCol + "                       ★ HOW TO PLAY ★                      " + bCol + "│"
        + reset + "\n";
    for (const auto& r : GamePresenter::getInstructionRules()) {
        const std::string col = m_truecolor ? appendTruecolorStr(r.color) : cCol;
        char hBuf[64];
        std::snprintf(hBuf, sizeof(hBuf), "  %-58s", r.header);
        frame += "  " + bCol + "│" + col + hBuf + bCol + "│" + reset + "\n";

        char dBuf[64];
        std::snprintf(dBuf, sizeof(dBuf), "    %-56s", r.detail);
        frame += "  " + bCol + "│" + dBuf + bCol + "│" + reset + "\n";
    }

    frame += "  " + bCol + "├────────────────────────────────────────────────────────────┤" + reset + "\n";
    frame += "  " + bCol + "│" + gCol + "           INSERT COIN - PRESS [SPACE] TO PLAY            " + bCol + "│"
        + reset + "\n";
    frame += "  " + bCol + "└────────────────────────────────────────────────────────────┘" + reset + "\n";
}

} // namespace qix::tui
