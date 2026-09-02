#include "RaylibApp.h"
#include <algorithm>

namespace qix::raylib {

RaylibApp::RaylibApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs) noexcept
    : m_game {std::move(game)}
    , m_delayMs {SpeedConfig::clampDelay(delayMs)}
{
}

bool RaylibApp::init(const std::string& title, int width, int height) noexcept
{
    return m_renderer.init(title, width, height);
}

std::uint32_t RaylibApp::getDelayMs() const noexcept
{
    return m_delayMs;
}

void RaylibApp::setDelayMs(std::uint32_t delayMs) noexcept
{
    m_delayMs = SpeedConfig::clampDelay(delayMs);
}

void RaylibApp::speedUp() noexcept
{
    setDelayMs(SpeedConfig::speedUp(m_delayMs));
}

void RaylibApp::speedDown() noexcept
{
    setDelayMs(SpeedConfig::speedDown(m_delayMs));
}

void RaylibApp::processInput() noexcept
{
    const auto view = m_game ? m_game->getView() : GameView {};

    // State screen transitions
    if (view.state == GameState::LevelComplete) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if (m_game) {
                m_game->nextLevel();
            }
            return;
        }
    } else if (view.state == GameState::GameOver) {
        if (IsKeyPressed(KEY_R)) {
            if (m_game) {
                m_game->reset();
            }
            return;
        }
    }

    // Direction input
    if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
        m_currentCmd.direction = Direction::Up;
    } else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
        m_currentCmd.direction = Direction::Down;
    } else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
        m_currentCmd.direction = Direction::Left;
    } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
        m_currentCmd.direction = Direction::Right;
    }

    // Draw mode input
    if (IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
        m_currentCmd.drawMode = DrawMode::Slow;
    } else if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_F)) {
        m_currentCmd.drawMode = DrawMode::Fast;
    } else {
        m_currentCmd.drawMode = DrawMode::None;
    }

    // Runtime speed adjustment
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_LEFT_BRACKET)) {
        speedDown();
    } else if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_RIGHT_BRACKET)) {
        speedUp();
    }

    // Reset game session
    if (IsKeyPressed(KEY_R)) {
        if (m_game) {
            m_game->reset();
        }
    }
}

void RaylibApp::run() noexcept
{
    double lastStepTime = GetTime();

    while (!WindowShouldClose()) {
        processInput();

        const double currentTime = GetTime();
        const double stepInterval = static_cast<double>(m_delayMs) / 1000.0;

        if (currentTime - lastStepTime >= stepInterval) {
            if (m_game) {
                m_game->handleInput(m_currentCmd);
                m_game->step(m_delayMs);
                m_currentCmd.direction = Direction::None;
            }
            lastStepTime = currentTime;
        }

        if (m_game) {
            m_renderer.render(m_game->getView(), m_delayMs);
        }
    }
}

} // namespace qix::raylib
