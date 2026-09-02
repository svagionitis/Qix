#include "MainWindow.h"
#include <QKeyEvent>

namespace qix::gui {

MainWindow::MainWindow(std::unique_ptr<IQixGame> game, QWidget* parent)
    : QMainWindow {parent}
    , m_game {std::move(game)}
{
    setWindowTitle("Qix Arcade (C++17)");
    resize(960, 720);

    m_canvas = new QixCanvas(this);
    setCentralWidget(m_canvas);

    // 60 FPS simulation and rendering loop
    connect(&m_timer, &QTimer::timeout, this, &MainWindow::onTick);
    m_timer.start(16);
}

void MainWindow::onTick()
{
    if (!m_game) {
        return;
    }

    m_game->handleInput(m_currentCmd);
    m_game->step(16);

    m_canvas->updateView(m_game->getView());

    // Clear direction after step
    m_currentCmd.direction = Direction::None;
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    const auto view = m_game->getView();

    // Restart or advance level on state screens
    if (view.state == GameState::LevelComplete) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
            m_game->nextLevel();
            return;
        }
    } else if (view.state == GameState::GameOver) {
        if (event->key() == Qt::Key_R) {
            m_game->reset();
            return;
        }
    }

    switch (event->key()) {
    case Qt::Key_Up:
    case Qt::Key_W:
        m_currentCmd.direction = Direction::Up;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        m_currentCmd.direction = Direction::Down;
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        m_currentCmd.direction = Direction::Left;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        m_currentCmd.direction = Direction::Right;
        break;
    case Qt::Key_Space:
    case Qt::Key_Control:
        m_currentCmd.drawMode = DrawMode::Slow;
        break;
    case Qt::Key_Shift:
    case Qt::Key_F:
        m_currentCmd.drawMode = DrawMode::Fast;
        break;
    case Qt::Key_R:
        m_game->reset();
        break;
    case Qt::Key_Escape:
        close();
        break;
    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Control || event->key() == Qt::Key_Shift
        || event->key() == Qt::Key_F) {
        m_currentCmd.drawMode = DrawMode::None;
    }

    QMainWindow::keyReleaseEvent(event);
}

} // namespace qix::gui
