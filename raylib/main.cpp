#include "GameConfig.h"
#include "QixGame.h"
#include "RaylibApp.h"
#include "SpeedConfig.h"
#include <memory>

int main(int argc, char* argv[])
{
    const auto delayMs = qix::SpeedConfig::parseSpeedArgs(argc, argv);
    const auto mode = qix::GameConfig::parseGameMode(argc, argv);
    const auto crtEnabled = qix::GameConfig::parseCrtFlag(argc, argv);
    const auto audioEnabled = qix::GameConfig::parseAudioFlag(argc, argv, false);
    const auto palette = qix::GameConfig::parsePaletteFlag(argc, argv);
    const auto attractMode = qix::GameConfig::parseAttractFlag(argc, argv);

    auto game = std::make_unique<qix::QixGame>(80, 60, 75, mode, delayMs);
    if (attractMode) {
        game->startAttractMode();
    }
    qix::raylib::RaylibApp app(std::move(game), delayMs, crtEnabled, audioEnabled, palette);

    if (!app.init("Qix Arcade (Raylib)", 960, 720)) {
        return 1;
    }

    app.run();
    return 0;
}
