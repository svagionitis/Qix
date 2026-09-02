#include "TuiRenderer.h"
#include <cstdio>
#include <iostream>
#include <string>

#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace qix::tui {

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

void TuiRenderer::render(const GameView& view, std::uint32_t delayMs) noexcept
{
    if (!view.playfield) {
        return;
    }

    // Move cursor to top-left
    std::string frame = "\033[H";

    // 1. HUD Header
    frame += "\033[1;36m=== QIX C++17 ARCADE ENGINE ===\033[0m\n";
    frame += "Score: \033[1;33m" + std::to_string(view.stats.score) + "\033[0m | ";
    frame += "Claimed: \033[1;32m" + std::to_string(view.stats.claimedPercent) + "% / "
        + std::to_string(view.stats.targetPercent) + "%\033[0m | ";
    frame += "Lives: \033[1;31m" + std::to_string(view.stats.lives) + "\033[0m | ";
    frame += "Level: \033[1;35m" + std::to_string(view.stats.level) + "\033[0m | ";
    frame += "Delay: \033[1;36m" + std::to_string(delayMs) + "ms\033[0m | ";

    std::string stateStr = "READY";
    if (view.state == GameState::Playing) {
        stateStr = "PLAYING";
    } else if (view.state == GameState::LevelComplete) {
        stateStr = "\033[1;32mVICTORY!\033[0m";
    } else if (view.state == GameState::GameOver) {
        stateStr = "\033[1;31mGAME OVER\033[0m";
    }
    frame += "State: " + stateStr + "\n";

    // 2. Playfield Grid
    const auto width = view.playfield->getWidth();
    const auto height = view.playfield->getHeight();

    // Downscale for standard terminal display if grid is large
    const std::int32_t stepX = (width > 60) ? 2 : 1;
    const std::int32_t stepY = (height > 30) ? 2 : 1;

    for (std::int32_t y {0}; y < height; y += stepY) {
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
            for (const auto& sp : view.sparxPositions) {
                if (p == sp) {
                    isSparx = true;
                    break;
                }
            }
            if (isSparx) {
                frame += "\033[1;35m$\033[0m";
                continue;
            }

            // Qix segments
            bool isQix = false;
            for (const auto& ribbon : view.qixRibbons) {
                for (const auto& seg : ribbon) {
                    const auto minX = std::min(seg.start.x, seg.end.x);
                    const auto maxX = std::max(seg.start.x, seg.end.x);
                    const auto minY = std::min(seg.start.y, seg.end.y);
                    const auto maxY = std::max(seg.start.y, seg.end.y);
                    if (x >= minX && x <= maxX && y >= minY && y <= maxY) {
                        isQix = true;
                        break;
                    }
                }
                if (isQix) {
                    break;
                }
            }
            if (isQix) {
                frame += "\033[1;31mX\033[0m";
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
        frame += "\n";
    }

    // 3. Controls Legend
    frame += "\033[2mControls: [WASD/Arrows] Move | [Space] Slow | [F] Fast | [-/+] Speed | [R] Reset | [Q] "
             "Quit\033[0m\n";

    std::cout << frame << std::flush;
}

PlayerCommand TuiRenderer::pollInput(TuiAction& action) noexcept
{
    PlayerCommand cmd {};
    action = TuiAction::None;

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
        default:
            break;
        }
    }

    return cmd;
}

} // namespace qix::tui
