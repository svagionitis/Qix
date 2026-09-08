#include "GameConfig.h"
#include "MainWindow.h"
#include "QixGame.h"
#include "SpeedConfig.h"
#include <QApplication>
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

    QApplication app(argc, argv);

    auto game = std::make_unique<qix::QixGame>(80, 60, 75, mode, delayMs);
    if (attractMode) {
        game->startAttractMode();
    }
    qix::qt::MainWindow window(std::move(game), delayMs, crtEnabled, audioEnabled, palette, artEnabled, artScene);
    window.show();

    return app.exec();
}
