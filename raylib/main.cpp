#include "QixGame.h"
#include "RaylibApp.h"
#include "SpeedConfig.h"
#include <memory>

int main(int argc, char* argv[])
{
    const auto delayMs = qix::SpeedConfig::parseSpeedArgs(argc, argv);

    auto game = std::make_unique<qix::QixGame>(80, 60, 75);
    qix::raylib::RaylibApp app(std::move(game), delayMs);

    if (!app.init("Qix Arcade (Raylib)", 960, 720)) {
        return 1;
    }

    app.run();
    return 0;
}
