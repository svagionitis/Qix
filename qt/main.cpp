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

    QApplication app(argc, argv);

    auto game = std::make_unique<qix::QixGame>(80, 60, 75, mode, delayMs);
    qix::qt::MainWindow window(std::move(game), delayMs, crtEnabled, audioEnabled);
    window.show();

    return app.exec();
}
