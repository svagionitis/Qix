#include "QixGame.h"
#include "TuiRenderer.h"
#include <algorithm>
#include <chrono>
#include <string>
#include <thread>

int main(int argc, char* argv[])
{
    // Default tick delay: 75 ms (~13.3 FPS) for comfortable terminal pacing
    std::uint32_t delayMs = 75;

    // Parse command line arguments for speed
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "--delay" || arg == "-d") && i + 1 < argc) {
            delayMs = static_cast<std::uint32_t>(std::stoul(argv[++i]));
        } else if ((arg == "--fps" || arg == "-f") && i + 1 < argc) {
            const auto fps = std::stoul(argv[++i]);
            if (fps > 0) {
                delayMs = static_cast<std::uint32_t>(1000 / fps);
            }
        }
    }

    // Clamp delay between 20ms (50 FPS) and 250ms (4 FPS)
    delayMs = std::clamp(delayMs, 20U, 250U);

    qix::QixGame game {60, 30, 75};
    qix::tui::TuiRenderer renderer {};

    renderer.init();

    bool running = true;
    qix::PlayerCommand currentCmd {};

    while (running) {
        qix::tui::TuiAction action = qix::tui::TuiAction::None;
        const auto cmd = renderer.pollInput(action);

        if (action == qix::tui::TuiAction::Quit) {
            running = false;
            break;
        }

        if (action == qix::tui::TuiAction::Restart) {
            game.reset();
        } else if (action == qix::tui::TuiAction::SpeedDown) {
            // Increase delay by 10ms (slower)
            delayMs = std::min(delayMs + 10U, 250U);
        } else if (action == qix::tui::TuiAction::SpeedUp) {
            // Decrease delay by 10ms (faster)
            delayMs = (delayMs > 25U) ? (delayMs - 10U) : 20U;
        }

        if (cmd.direction != qix::Direction::None) {
            currentCmd.direction = cmd.direction;
        }
        if (cmd.drawMode != qix::DrawMode::None) {
            currentCmd.drawMode = cmd.drawMode;
        }

        game.handleInput(currentCmd);
        game.step(delayMs);

        renderer.render(game.getView(), delayMs);

        // Reset direction after step
        currentCmd.direction = qix::Direction::None;

        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }

    renderer.shutdown();
    return 0;
}
