#include "ArcadeAudio.h"
#include "GameConfig.h"
#include "QixGame.h"
#include "ReplaySystem.h"
#include "SpeedConfig.h"
#include "TuiAudio.h"
#include "TuiRenderer.h"
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <thread>

int main(int argc, char* argv[])
{
    std::uint32_t delayMs = qix::SpeedConfig::parseSpeedArgs(argc, argv, 40U);
    const auto mode = qix::GameConfig::parseGameMode(argc, argv);
    const auto palette = qix::GameConfig::parsePaletteFlag(argc, argv);
    const auto artEnabled = qix::GameConfig::parseArtFlag(argc, argv, true);
    const auto artScene = qix::GameConfig::parseArtSceneFlag(argc, argv, -1);
    const auto recordPath = qix::GameConfig::parseRecordFlag(argc, argv);
    const auto replayPath = qix::GameConfig::parseReplayFlag(argc, argv);
    const auto audioEnabled = qix::GameConfig::parseAudioFlag(argc, argv, false);

    qix::ArcadeAudio audio {};
    audio.setMuted(!audioEnabled);
    qix::tui::TuiAudioStream audioStream {audio};
    audioStream.start();

    bool brailleMode = true;
    bool truecolor = true;
    bool diffUpdates = true;
    std::int32_t customWidth = 0;
    std::int32_t customHeight = 0;

    for (int i = 1; i < argc; ++i) {
        if (argv[i] != nullptr) {
            const std::string arg {argv[i]};
            if (arg == "--ascii") {
                brailleMode = false;
            } else if (arg == "--braille") {
                brailleMode = true;
            } else if (arg == "--no-truecolor" || arg == "--no-rgb") {
                truecolor = false;
            } else if (arg == "--truecolor" || arg == "--rgb") {
                truecolor = true;
            } else if (arg == "--no-diff" || arg == "--no-differential") {
                diffUpdates = false;
            } else if (arg == "--diff" || arg == "--differential") {
                diffUpdates = true;
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

    const auto attractMode = qix::GameConfig::parseAttractFlag(argc, argv);
    const auto demoDurationMs = qix::GameConfig::parseDemoDurationFlag(argc, argv);
    const auto randomSpawns = qix::GameConfig::parseRandomSpawnsFlag(argc, argv, mode == qix::GameMode::Modern);
    std::unique_ptr<qix::QixGame> game;
    qix::ReplayPlayer player;
    bool replaying = false;

    if (!replayPath.empty()) {
        if (player.loadFromFile(replayPath)) {
            const auto& hdr = player.getHeader();
            width = hdr.playfieldWidth;
            height = hdr.playfieldHeight;
            game = std::make_unique<qix::QixGame>(width, height, hdr.targetPercent, hdr.mode, hdr.baseDelayMs);
            game->setDemoDurationMs(demoDurationMs);
            delayMs = hdr.baseDelayMs;
            replaying = true;
        } else {
            std::cerr << "[Qix] Failed to load replay file: " << replayPath << "\n";
            return 1;
        }
    } else {
        game = std::make_unique<qix::QixGame>(width, height, 75, mode, delayMs, randomSpawns);
        game->setDemoDurationMs(demoDurationMs);
        if (attractMode) {
            game->startAttractMode();
        }
    }

    qix::ReplayRecorder recorder;
    std::uint32_t simTick = 0;
    if (!recordPath.empty() && !replaying) {
        const auto& view = game->getView();
        qix::ReplayHeader hdr {};
        hdr.mode = game->getGameMode();
        hdr.playfieldWidth = (view.playfield != nullptr) ? view.playfield->getWidth() : width;
        hdr.playfieldHeight = (view.playfield != nullptr) ? view.playfield->getHeight() : height;
        hdr.targetPercent = view.stats.targetPercent;
        hdr.baseDelayMs = game->getBaseDelayMs();
        recorder.start(hdr);
    }

    qix::tui::TuiRenderer renderer {};
    renderer.setBrailleMode(brailleMode);
    renderer.setTruecolor(truecolor);
    renderer.setDifferentialUpdates(diffUpdates);
    renderer.setPalette(palette);
    renderer.setArtEnabled(artEnabled);
    renderer.setArtScene(artScene);
    renderer.setAudioMuted(audio.isMuted());

    renderer.init();

    bool running = true;
    qix::PlayerCommand currentCmd {};

    while (running) {
        const auto frameStart = std::chrono::steady_clock::now();
        qix::tui::TuiAction action = qix::tui::TuiAction::None;
        const auto cmd = renderer.pollInput(action);

        if (action == qix::tui::TuiAction::Resize) {
            if (!customSizeSpecified && game->getView().state == qix::GameState::Ready) {
                const auto [newW, newH] = qix::tui::TuiRenderer::computePlayfieldDimensions(renderer.isBrailleMode());
                game = std::make_unique<qix::QixGame>(newW, newH, 75, mode, delayMs);
            }
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (game->getView().state == qix::GameState::Attract) {
            if (action != qix::tui::TuiAction::None || cmd.direction != qix::Direction::None
                || cmd.drawMode != qix::DrawMode::None) {
                game->exitAttractMode();
                renderer.render(game->getView(), delayMs);
                continue;
            }
        }

        if (game->getView().state == qix::GameState::NameEntry) {
            if (action == qix::tui::TuiAction::Backspace) {
                game->handleInput(qix::PlayerCommand {qix::Direction::Left, qix::DrawMode::None});
            } else if (action == qix::tui::TuiAction::Confirm
                || (action == qix::tui::TuiAction::None && cmd.drawMode == qix::DrawMode::Slow)) {
                if (game->getView().nameEntry.cursorIndex < 2) {
                    game->handleInput(qix::PlayerCommand {qix::Direction::Right, qix::DrawMode::None});
                } else {
                    game->confirmInitials();
                }
            } else if (action == qix::tui::TuiAction::CharInput || action == qix::tui::TuiAction::Quit
                || action == qix::tui::TuiAction::Restart || action == qix::tui::TuiAction::ToggleBraille
                || action == qix::tui::TuiAction::ToggleTruecolor || action == qix::tui::TuiAction::CyclePalette
                || action == qix::tui::TuiAction::DisengageDraw || cmd.drawMode == qix::DrawMode::Fast) {
                const char typed = renderer.getTypedChar();
                if (typed != 0) {
                    game->inputInitialsChar(typed);
                }
            } else if (cmd.direction != qix::Direction::None) {
                game->handleInput(cmd);
            }
            renderer.render(game->getView(), delayMs);
            const auto frameElapsed
                = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - frameStart)
                      .count();
            if (static_cast<std::uint32_t>(frameElapsed) < delayMs) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(delayMs - static_cast<std::uint32_t>(frameElapsed)));
            }
            continue;
        }

        if (action == qix::tui::TuiAction::Quit) {
            running = false;
            break;
        }

        if (action == qix::tui::TuiAction::ToggleBraille) {
            renderer.toggleBrailleMode();
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (action == qix::tui::TuiAction::ToggleTruecolor) {
            renderer.toggleTruecolor();
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (action == qix::tui::TuiAction::CyclePalette) {
            renderer.cyclePalette();
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (action == qix::tui::TuiAction::ToggleArt) {
            renderer.toggleArt();
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (action == qix::tui::TuiAction::ToggleAudio) {
            audio.toggleMute();
            renderer.setAudioMuted(audio.isMuted());
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (action == qix::tui::TuiAction::QuickSave) {
            (void)game->quickSave();
            continue;
        }

        if (action == qix::tui::TuiAction::QuickLoad) {
            if (game->quickLoad()) {
                delayMs = game->getCurrentDelayMs();
            }
            audio.update(game->getView(), delayMs);
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (action == qix::tui::TuiAction::TogglePause) {
            game->togglePause();
            audio.update(game->getView(), delayMs);
            renderer.render(game->getView(), delayMs);
            continue;
        }

        if (game->getView().state == qix::GameState::LevelComplete) {
            audio.update(game->getView(), delayMs);
            if (cmd.drawMode == qix::DrawMode::Slow || cmd.direction != qix::Direction::None) {
                game->nextLevel();
                delayMs = game->getCurrentDelayMs();
                audio.update(game->getView(), delayMs);
                continue;
            }
        } else if (game->getView().state == qix::GameState::HallOfFame
            || game->getView().state == qix::GameState::GameOver) {
            audio.update(game->getView(), delayMs);
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
                audio.update(game->getView(), delayMs);
            }
            renderer.render(game->getView(), delayMs);
            const auto frameElapsed
                = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - frameStart)
                      .count();
            if (static_cast<std::uint32_t>(frameElapsed) < delayMs) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(delayMs - static_cast<std::uint32_t>(frameElapsed)));
            }
            continue;
        }

        if (action == qix::tui::TuiAction::Restart) {
            if (!customSizeSpecified) {
                const auto [newW, newH] = qix::tui::TuiRenderer::computePlayfieldDimensions(renderer.isBrailleMode());
                game = std::make_unique<qix::QixGame>(newW, newH, 75, mode, delayMs);
            } else {
                game->reset();
            }
            if (recorder.isRecording()) {
                const auto& view = game->getView();
                qix::ReplayHeader hdr {};
                hdr.mode = game->getGameMode();
                hdr.playfieldWidth = (view.playfield != nullptr) ? view.playfield->getWidth() : width;
                hdr.playfieldHeight = (view.playfield != nullptr) ? view.playfield->getHeight() : height;
                hdr.targetPercent = view.stats.targetPercent;
                hdr.baseDelayMs = game->getBaseDelayMs();
                recorder.start(hdr);
                simTick = 0;
            }
            delayMs = game->getCurrentDelayMs();
            audio.update(game->getView(), delayMs);
        } else if (action == qix::tui::TuiAction::SpeedDown) {
            game->setBaseDelayMs(qix::SpeedConfig::speedDown(game->getBaseDelayMs()));
            delayMs = game->getCurrentDelayMs();
        } else if (action == qix::tui::TuiAction::SpeedUp) {
            game->setBaseDelayMs(qix::SpeedConfig::speedUp(game->getBaseDelayMs()));
            delayMs = game->getCurrentDelayMs();
        } else if (action == qix::tui::TuiAction::DisengageDraw) {
            currentCmd.drawMode = qix::DrawMode::None;
        }

        if (replaying) {
            currentCmd = player.getCommandForTick(simTick);
            if (game->getView().state == qix::GameState::LevelComplete) {
                if (currentCmd.drawMode != qix::DrawMode::None || currentCmd.direction != qix::Direction::None) {
                    game->nextLevel();
                    delayMs = game->getCurrentDelayMs();
                }
            }
        } else {
            if (cmd.direction != qix::Direction::None) {
                currentCmd.direction = cmd.direction;
            }
            if (cmd.drawMode != qix::DrawMode::None) {
                currentCmd.drawMode = cmd.drawMode;
            }
            if (recorder.isRecording()) {
                recorder.recordTick(simTick, currentCmd);
            }
        }

        const bool wasDrawing = (game->getView().drawMode != qix::DrawMode::None);

        game->handleInput(currentCmd);
        game->step(delayMs);
        ++simTick;

        audio.update(game->getView(), delayMs);

        const bool isDrawing = (game->getView().drawMode != qix::DrawMode::None);
        if (wasDrawing && !isDrawing) {
            currentCmd.drawMode = qix::DrawMode::None;
        }

        renderer.render(game->getView(), delayMs);

        // Reset direction after step
        currentCmd.direction = qix::Direction::None;

        const auto frameElapsed
            = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - frameStart)
                  .count();
        if (static_cast<std::uint32_t>(frameElapsed) < delayMs) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs - static_cast<std::uint32_t>(frameElapsed)));
        }
    }

    if (recorder.isRecording() && !recordPath.empty()) {
        recorder.finish(game->getView().stats.score, simTick);
        static_cast<void>(recorder.saveToFile(recordPath));
    }

    audioStream.stop();
    renderer.shutdown();
    return 0;
}
