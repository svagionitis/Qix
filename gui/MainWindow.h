#ifndef QIX_GUI_MAIN_WINDOW_H
#define QIX_GUI_MAIN_WINDOW_H

#include "IQixGame.h"
#include "QixCanvas.h"
#include <QMainWindow>
#include <QTimer>
#include <memory>

namespace qix::gui {

/// @class MainWindow
/// @brief Main desktop window managing game loop pacing and keyboard input routing.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::unique_ptr<IQixGame> game, QWidget* parent = nullptr);
    ~MainWindow() override = default;

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onTick();

private:
    std::unique_ptr<IQixGame> m_game;
    QixCanvas* m_canvas {nullptr};
    QTimer m_timer;
    PlayerCommand m_currentCmd {};
};

} // namespace qix::gui

#endif // QIX_GUI_MAIN_WINDOW_H
