#include "GameConfig.h"
#include "QixGame.h"
#include "SpeedConfig.h"
#include "TuiRenderer.h"
#include <chrono>
#include <cstdlib>
#include <memory>
#include <thread>

int main(int argc, char* argv[])
{
    std::uint32_t delayMs = qix::SpeedConfig::parseSpeedArgs(argc, argv);
    const auto mode = qix::GameConfig::parseGameMode(argc, argv);

    bool brailleMode = true;
    std::int32_t customWidth = 0;
    std::int32_t customHeight = 0;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr) {
            const std::string arg {argv[i]};
            if (arg == "--ascii") {
                brailleMode = false;
            } else if (arg == "--braille") {
                brailleMode = true;
            } else if ((arg == "--width" || arg == "-w") && i + 1 < argc && argv[i + 1] != nullptr) {
                customWidth = std::atoi(argv[++i]);
            } else if ((arg == "--height" || arg == "-h") && i + 1 < argc && argv[i + 1] != nullptr) {
                customHeight = std::atoi(argv[++i]);
            }
        }
    }

    const bool customSizeSpecified = (customWidth > 0 && customHeight > 0);
    std::int32_t width = customWidth;
    std::int32_t height = customHeight;
    if (!customSizeSpecified) {
        const auto [autoW, autoH] = qix::tui::TuiRenderer::computePlayfieldDimensions(brailleMode);
        width = autoW;
        height = autoH;
    }

    auto game = std::make_unique<qix::QixGame>(width, height, 75, mode, delayMs);
    qix::tui::TuiRenderer renderer {};
    renderer.setBrailleMode(brailleMode);

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

        if (action == qix::tui::TuiAction::ToggleBraille) {
            renderer.toggleBrailleMode();
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (action == qix::tui::TuiAction::Resize) {
            if (!customSizeSpecified && game->getView().state == qix::GameState::Ready) {
                const auto [newW, newH] = qix::tui::TuiRenderer::computePlayfieldDimensions(renderer.isBrailleMode());
                game = std::make_unique<qix::QixGame>(newW, newH, 75, mode, delayMs);
            }
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (game->getView().state == qix::GameState::LevelComplete) {
            if (cmd.drawMode == qix::DrawMode::Slow || cmd.direction != qix::Direction::None) {
                game->nextLevel();
                delayMs = game->getCurrentDelayMs();
                continue;
            }
        } else if (game->getView().state == qix::GameState::NameEntry) {
            if (action == qix::tui::TuiAction::Confirm || cmd.drawMode != qix::DrawMode::None) {
                if (game->getView().nameEntry.cursorIndex < 2) {
                    game->handleInput(qix::PlayerCommand {qix::Direction::Right, qix::DrawMode::None});
                } else {
                    game->confirmInitials();
                }
            } else if (cmd.direction != qix::Direction::None) {
                game->handleInput(cmd);
            }
            renderer.render(game->getView(), delayMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            continue;
        } else if (game->getView().state == qix::GameState::HallOfFame
            || game->getView().state == qix::GameState::GameOver) {
            if (action == qix::tui::TuiAction::Restart || action == qix::tui::TuiAction::Confirm
                || cmd.drawMode != qix::DrawMode::None) {
                if (!customSizeSpecified) {
                    const auto [newW, newH]
                        = qix::tui::TuiRenderer::computePlayfieldDimensions(renderer.isBrailleMode());
                    game = std::make_unique<qix::QixGame>(newW, newH, 75, mode, delayMs);
                } else {
                    game->reset();
                }
                delayMs = game->getCurrentDelayMs();
            }
            renderer.render(game->getView(), delayMs);
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
            continue;
        }

        if (action == qix::tui::TuiAction::Restart) {
            if (!customSizeSpecified) {
                const auto [newW, newH] = qix::tui::TuiRenderer::computePlayfieldDimensions(renderer.isBrailleMode());
                game = std::make_unique<qix::QixGame>(newW, newH, 75, mode, delayMs);
            } else {
                game->reset();
            }
            delayMs = game->getCurrentDelayMs();
        } else if (action == qix::tui::TuiAction::SpeedDown) {
            game->setBaseDelayMs(qix::SpeedConfig::speedDown(game->getBaseDelayMs()));
            delayMs = game->getCurrentDelayMs();
        } else if (action == qix::tui::TuiAction::SpeedUp) {
            game->setBaseDelayMs(qix::SpeedConfig::speedUp(game->getBaseDelayMs()));
            delayMs = game->getCurrentDelayMs();
        } else if (action == qix::tui::TuiAction::DisengageDraw) {
            currentCmd.drawMode = qix::DrawMode::None;
        }

        if (cmd.direction != qix::Direction::None) {
            currentCmd.direction = cmd.direction;
        }
        if (cmd.drawMode != qix::DrawMode::None) {
            currentCmd.drawMode = cmd.drawMode;
        }

        const bool wasDrawing = (game->getView().drawMode != qix::DrawMode::None);

        game->handleInput(currentCmd);
        game->step(delayMs);

        const bool isDrawing = (game->getView().drawMode != qix::DrawMode::None);
        if (wasDrawing && !isDrawing) {
            currentCmd.drawMode = qix::DrawMode::None;
        }

        renderer.render(game->getView(), delayMs);

        // Reset direction after step
        currentCmd.direction = qix::Direction::None;

        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
    }

    renderer.shutdown();
    return 0;
}
