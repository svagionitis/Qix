#include "GameConfig.h"
#include "QixGame.h"
#include "SpeedConfig.h"
#include "TuiRenderer.h"
#include <chrono>
#include <thread>

int main(int argc, char* argv[])
{
    std::uint32_t delayMs = qix::SpeedConfig::parseSpeedArgs(argc, argv);
    const auto mode = qix::GameConfig::parseGameMode(argc, argv);

    qix::QixGame game {60, 30, 75, mode, delayMs};
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

        if (game.getView().state == qix::GameState::LevelComplete) {
            if (cmd.drawMode == qix::DrawMode::Slow || cmd.direction != qix::Direction::None) {
                game.nextLevel();
                delayMs = game.getCurrentDelayMs();
                continue;
            }
        }

        if (action == qix::tui::TuiAction::Restart) {
            game.reset();
            delayMs = game.getCurrentDelayMs();
        } else if (action == qix::tui::TuiAction::SpeedDown) {
            game.setBaseDelayMs(qix::SpeedConfig::speedDown(game.getBaseDelayMs()));
            delayMs = game.getCurrentDelayMs();
        } else if (action == qix::tui::TuiAction::SpeedUp) {
            game.setBaseDelayMs(qix::SpeedConfig::speedUp(game.getBaseDelayMs()));
            delayMs = game.getCurrentDelayMs();
        } else if (action == qix::tui::TuiAction::DisengageDraw) {
            currentCmd.drawMode = qix::DrawMode::None;
        }

        if (cmd.direction != qix::Direction::None) {
            currentCmd.direction = cmd.direction;
        }
        if (cmd.drawMode != qix::DrawMode::None) {
            currentCmd.drawMode = cmd.drawMode;
        }

        const bool wasDrawing = (game.getView().drawMode != qix::DrawMode::None);

        game.handleInput(currentCmd);
        game.step(delayMs);

        const bool isDrawing = (game.getView().drawMode != qix::DrawMode::None);
        if (wasDrawing && !isDrawing) {
            currentCmd.drawMode = qix::DrawMode::None;
        }

        renderer.render(game.getView(), delayMs);

        // Reset direction after step
        currentCmd.direction = qix::Direction::None;

        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }

    renderer.shutdown();
    return 0;
}
