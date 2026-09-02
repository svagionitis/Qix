#include "QixGame.h"
#include "SpeedConfig.h"
#include "TuiRenderer.h"
#include <chrono>
#include <thread>

int main(int argc, char* argv[])
{
    std::uint32_t delayMs = qix::SpeedConfig::parseSpeedArgs(argc, argv);

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
            delayMs = qix::SpeedConfig::speedDown(delayMs);
        } else if (action == qix::tui::TuiAction::SpeedUp) {
            delayMs = qix::SpeedConfig::speedUp(delayMs);
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
