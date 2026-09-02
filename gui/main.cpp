#include "MainWindow.h"
#include "QixGame.h"
#include "SpeedConfig.h"
#include <QApplication>
#include <memory>

int main(int argc, char* argv[])
{
    const auto delayMs = qix::SpeedConfig::parseSpeedArgs(argc, argv);

    QApplication app(argc, argv);

    auto game = std::make_unique<qix::QixGame>(80, 60, 75);
    qix::gui::MainWindow window(std::move(game), delayMs);
    window.show();

    return app.exec();
}
