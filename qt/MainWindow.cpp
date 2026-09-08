#include "MainWindow.h"
#include <QKeyEvent>

namespace qix::qt {

MainWindow::MainWindow(std::unique_ptr<IQixGame> game, std::uint32_t delayMs, QWidget* parent)
    : QMainWindow {parent}
    , m_game {std::move(game)}
    , m_delayMs {SpeedConfig::clampDelay(delayMs)}
{
    setWindowTitle("Qix Arcade (C++17)");
    resize(960, 720);

    m_canvas = new QixCanvas(this);
    m_canvas->setDelayMs(m_delayMs);
    setCentralWidget(m_canvas);

    // Dynamic simulation and rendering loop
    connect(&m_timer, &QTimer::timeout, this, &MainWindow::onTick);
    m_timer.start(static_cast<int>(m_delayMs));
}

std::uint32_t MainWindow::getDelayMs() const noexcept
{
    return m_delayMs;
}

void MainWindow::setDelayMs(std::uint32_t delayMs) noexcept
{
    m_delayMs = SpeedConfig::clampDelay(delayMs);
    if (m_timer.isActive()) {
        m_timer.setInterval(static_cast<int>(m_delayMs));
    }
    if (m_canvas != nullptr) {
        m_canvas->setDelayMs(m_delayMs);
    }
}

void MainWindow::speedUp() noexcept
{
    if (m_game) {
        m_game->setBaseDelayMs(SpeedConfig::speedUp(m_game->getBaseDelayMs()));
        setDelayMs(m_game->getCurrentDelayMs());
    } else {
        setDelayMs(SpeedConfig::speedUp(m_delayMs));
    }
}

void MainWindow::speedDown() noexcept
{
    if (m_game) {
        m_game->setBaseDelayMs(SpeedConfig::speedDown(m_game->getBaseDelayMs()));
        setDelayMs(m_game->getCurrentDelayMs());
    } else {
        setDelayMs(SpeedConfig::speedDown(m_delayMs));
    }
}

void MainWindow::onTick()
{
    if (!m_game) {
        return;
    }

    m_game->handleInput(m_currentCmd);
    m_game->step(m_delayMs);

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
            setDelayMs(m_game->getCurrentDelayMs());
            m_canvas->updateView(m_game->getView());
            return;
        }
    } else if (view.state == GameState::NameEntry) {
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W) {
            m_game->handleInput(PlayerCommand {Direction::Up, DrawMode::None});
            m_canvas->updateView(m_game->getView());
            return;
        }
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S) {
            m_game->handleInput(PlayerCommand {Direction::Down, DrawMode::None});
            m_canvas->updateView(m_game->getView());
            return;
        }
        if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
            m_game->handleInput(PlayerCommand {Direction::Left, DrawMode::None});
            m_canvas->updateView(m_game->getView());
            return;
        }
        if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
            m_game->handleInput(PlayerCommand {Direction::Right, DrawMode::None});
            m_canvas->updateView(m_game->getView());
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) {
            if (view.nameEntry.cursorIndex < 2) {
                m_game->handleInput(PlayerCommand {Direction::Right, DrawMode::None});
            } else {
                m_game->confirmInitials();
            }
            m_canvas->updateView(m_game->getView());
            return;
        }
        const QString text = event->text();
        if (!text.isEmpty()) {
            const QChar ch = text.at(0);
            if (ch.isLetterOrNumber()) {
                m_game->inputInitialsChar(static_cast<char>(ch.toUpper().toLatin1()));
                m_canvas->updateView(m_game->getView());
                return;
            }
        }
        return;
    } else if (view.state == GameState::HallOfFame || view.state == GameState::GameOver) {
        if (event->key() == Qt::Key_R || event->key() == Qt::Key_Space || event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter) {
            m_game->reset();
            setDelayMs(m_game->getCurrentDelayMs());
            m_canvas->updateView(m_game->getView());
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
    case Qt::Key_Minus:
    case Qt::Key_BracketLeft:
    case Qt::Key_Underscore:
        speedDown();
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
    case Qt::Key_BracketRight:
        speedUp();
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

} // namespace qix::qt
