#include "QixGame.h"
#include "SdlApp.h"
#include "SpeedConfig.h"
#include <memory>

int main(int argc, char* argv[])
{
    const auto delayMs = qix::SpeedConfig::parseSpeedArgs(argc, argv);

    auto game = std::make_unique<qix::QixGame>(80, 60, 75);
    qix::sdl::SdlApp app(std::move(game), delayMs);

    if (!app.init("Qix Arcade (SDL2)", 960, 720)) {
        return 1;
    }

    app.run();
    return 0;
}
