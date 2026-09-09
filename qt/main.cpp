#include "GameConfig.h"
#include "MainWindow.h"
#include "QixGame.h"
#include "ReplaySystem.h"
#include "SpeedConfig.h"
#include <QApplication>
#include <iostream>
#include <memory>

int main(int argc, char* argv[])
{
    const auto delayMs = qix::SpeedConfig::parseSpeedArgs(argc, argv);
    const auto mode = qix::GameConfig::parseGameMode(argc, argv);
    const auto crtEnabled = qix::GameConfig::parseCrtFlag(argc, argv);
    const auto audioEnabled = qix::GameConfig::parseAudioFlag(argc, argv, false);
    const auto palette = qix::GameConfig::parsePaletteFlag(argc, argv);
    const auto attractMode = qix::GameConfig::parseAttractFlag(argc, argv);
    const auto artEnabled = qix::GameConfig::parseArtFlag(argc, argv, true);
    const auto artScene = qix::GameConfig::parseArtSceneFlag(argc, argv, -1);
    const auto recordPath = qix::GameConfig::parseRecordFlag(argc, argv);
    const auto replayPath = qix::GameConfig::parseReplayFlag(argc, argv);
    const auto demoDurationMs = qix::GameConfig::parseDemoDurationFlag(argc, argv);

    QApplication app(argc, argv);

    std::unique_ptr<qix::QixGame> game;
    qix::ReplayPlayer player;
    bool replaying = false;

    if (!replayPath.empty()) {
        if (player.loadFromFile(replayPath)) {
            const auto& hdr = player.getHeader();
            game = std::make_unique<qix::QixGame>(
                hdr.playfieldWidth, hdr.playfieldHeight, hdr.targetPercent, hdr.mode, hdr.baseDelayMs);
            game->setDemoDurationMs(demoDurationMs);
            replaying = true;
        } else {
            std::cerr << "[Qix] Failed to load replay file: " << replayPath << "\n";
            return 1;
        }
    } else {
        game = std::make_unique<qix::QixGame>(80, 60, 75, mode, delayMs);
        game->setDemoDurationMs(demoDurationMs);
        if (attractMode) {
            game->startAttractMode();
        }
    }

    const auto activeDelay = replaying ? player.getHeader().baseDelayMs : delayMs;
    qix::qt::MainWindow window(std::move(game), activeDelay, crtEnabled, audioEnabled, palette, artEnabled, artScene);

    if (replaying) {
        window.setReplayPlayer(std::move(player));
    } else if (!recordPath.empty()) {
        window.setRecordPath(recordPath);
    }

    window.show();

    return app.exec();
}
