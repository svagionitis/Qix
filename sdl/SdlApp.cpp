#include "SdlApp.h"
#include <algorithm>

namespace qix::sdl {

SdlApp::SdlApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs) noexcept
    : m_game {std::move(game)}
    , m_delayMs {SpeedConfig::clampDelay(delayMs)}
{
}

SdlApp::~SdlApp()
{
    if (m_sdlInitialized) {
        SDL_Quit();
    }
}

bool SdlApp::init(const std::string& title, int width, int height) noexcept
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        return false;
    }
    m_sdlInitialized = true;

    return m_renderer.init(title, width, height);
}

std::uint32_t SdlApp::getDelayMs() const noexcept
{
    return m_delayMs;
}

void SdlApp::setDelayMs(std::uint32_t delayMs) noexcept
{
    m_delayMs = SpeedConfig::clampDelay(delayMs);
}

void SdlApp::speedUp() noexcept
{
    setDelayMs(SpeedConfig::speedUp(m_delayMs));
}

void SdlApp::speedDown() noexcept
{
    setDelayMs(SpeedConfig::speedDown(m_delayMs));
}

void SdlApp::processEvents(bool& running) noexcept
{
    SDL_Event event {};
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            running = false;
            return;
        }

        if (event.type == SDL_KEYDOWN) {
            const auto key = event.key.keysym.sym;
            const auto view = m_game ? m_game->getView() : GameView {};

            // State screen transitions
            if (view.state == GameState::LevelComplete) {
                if (key == SDLK_SPACE || key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    if (m_game) {
                        m_game->nextLevel();
                    }
                    continue;
                }
            } else if (view.state == GameState::GameOver) {
                if (key == SDLK_r) {
                    if (m_game) {
                        m_game->reset();
                    }
                    continue;
                }
            }

            switch (key) {
            case SDLK_UP:
            case SDLK_w:
                m_currentCmd.direction = Direction::Up;
                break;
            case SDLK_DOWN:
            case SDLK_s:
                m_currentCmd.direction = Direction::Down;
                break;
            case SDLK_LEFT:
            case SDLK_a:
                m_currentCmd.direction = Direction::Left;
                break;
            case SDLK_RIGHT:
            case SDLK_d:
                m_currentCmd.direction = Direction::Right;
                break;
            case SDLK_SPACE:
            case SDLK_LCTRL:
            case SDLK_RCTRL:
                m_currentCmd.drawMode = DrawMode::Slow;
                break;
            case SDLK_LSHIFT:
            case SDLK_RSHIFT:
            case SDLK_f:
                m_currentCmd.drawMode = DrawMode::Fast;
                break;
            case SDLK_MINUS:
            case SDLK_KP_MINUS:
            case SDLK_LEFTBRACKET:
            case SDLK_UNDERSCORE:
                speedDown();
                break;
            case SDLK_PLUS:
            case SDLK_KP_PLUS:
            case SDLK_EQUALS:
            case SDLK_RIGHTBRACKET:
                speedUp();
                break;
            case SDLK_r:
                if (m_game) {
                    m_game->reset();
                }
                break;
            case SDLK_ESCAPE:
                running = false;
                break;
            default:
                break;
            }
        } else if (event.type == SDL_KEYUP) {
            const auto key = event.key.keysym.sym;
            if (key == SDLK_SPACE || key == SDLK_LCTRL || key == SDLK_RCTRL || key == SDLK_LSHIFT || key == SDLK_RSHIFT
                || key == SDLK_f) {
                m_currentCmd.drawMode = DrawMode::None;
            }
        }
    }
}

void SdlApp::run() noexcept
{
    bool running {true};

    while (running) {
        const auto frameStart = SDL_GetTicks();

        processEvents(running);
        if (!running) {
            break;
        }

        if (m_game) {
            m_game->handleInput(m_currentCmd);
            m_game->step(m_delayMs);

            m_renderer.render(m_game->getView(), m_delayMs);
            m_renderer.present();

            // Clear direction after step
            m_currentCmd.direction = Direction::None;
        }

        const auto frameElapsed = SDL_GetTicks() - frameStart;
        if (frameElapsed < m_delayMs) {
            SDL_Delay(m_delayMs - frameElapsed);
        }
    }
}

} // namespace qix::sdl
