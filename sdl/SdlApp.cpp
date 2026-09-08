#include "SdlApp.h"
#include <algorithm>

namespace qix::sdl {

SdlApp::SdlApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs, bool crtEnabled, bool audioEnabled) noexcept
    : m_game {std::move(game)}
    , m_delayMs {SpeedConfig::clampDelay(delayMs)}
{
    m_renderer.setCrtEnabled(crtEnabled);
    m_audio.setMuted(!audioEnabled);
}

SdlApp::~SdlApp()
{
    if (m_audioDevice != 0) {
        SDL_CloseAudioDevice(m_audioDevice);
        m_audioDevice = 0;
    }
    if (m_sdlInitialized) {
        SDL_Quit();
    }
}

bool SdlApp::init(const std::string& title, int width, int height) noexcept
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        return false;
    }
    m_sdlInitialized = true;

    // Open audio device
    SDL_AudioSpec desired {};
    desired.freq = static_cast<int>(ArcadeAudio::SampleRate);
    desired.format = AUDIO_S16SYS;
    desired.channels = 1;
    desired.samples = 1024;
    desired.callback = sdlAudioCallback;
    desired.userdata = this;

    SDL_AudioSpec obtained {};
    m_audioDevice = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (m_audioDevice != 0) {
        SDL_PauseAudioDevice(m_audioDevice, 0);
    }

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
    if (m_game) {
        m_game->setBaseDelayMs(SpeedConfig::speedUp(m_game->getBaseDelayMs()));
        setDelayMs(m_game->getCurrentDelayMs());
    } else {
        setDelayMs(SpeedConfig::speedUp(m_delayMs));
    }
}

void SdlApp::speedDown() noexcept
{
    if (m_game) {
        m_game->setBaseDelayMs(SpeedConfig::speedDown(m_game->getBaseDelayMs()));
        setDelayMs(m_game->getCurrentDelayMs());
    } else {
        setDelayMs(SpeedConfig::speedDown(m_delayMs));
    }
}

void SdlApp::setCrtEnabled(bool enabled) noexcept
{
    m_renderer.setCrtEnabled(enabled);
}

bool SdlApp::isCrtEnabled() const noexcept
{
    return m_renderer.isCrtEnabled();
}

void SdlApp::toggleCrt() noexcept
{
    m_renderer.toggleCrt();
}

void SdlApp::setAudioEnabled(bool enabled) noexcept
{
    m_audio.setMuted(!enabled);
}

bool SdlApp::isAudioEnabled() const noexcept
{
    return !m_audio.isMuted();
}

void SdlApp::toggleAudio() noexcept
{
    m_audio.toggleMute();
}

void SdlApp::sdlAudioCallback(void* userdata, Uint8* stream, int len) noexcept
{
    auto* app = static_cast<SdlApp*>(userdata);
    if (app != nullptr && stream != nullptr && len > 0) {
        app->m_audio.generateSamples(
            reinterpret_cast<std::int16_t*>(stream), static_cast<std::size_t>(len) / sizeof(std::int16_t));
    } else if (stream != nullptr && len > 0) {
        std::fill_n(stream, len, static_cast<Uint8>(0));
    }
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
                        setDelayMs(m_game->getCurrentDelayMs());
                    }
                    continue;
                }
            } else if (view.state == GameState::NameEntry) {
                if (key == SDLK_UP || key == SDLK_w) {
                    if (m_game) {
                        m_game->handleInput(PlayerCommand {Direction::Up, DrawMode::None});
                    }
                    continue;
                }
                if (key == SDLK_DOWN || key == SDLK_s) {
                    if (m_game) {
                        m_game->handleInput(PlayerCommand {Direction::Down, DrawMode::None});
                    }
                    continue;
                }
                if (key == SDLK_LEFT || key == SDLK_a) {
                    if (m_game) {
                        m_game->handleInput(PlayerCommand {Direction::Left, DrawMode::None});
                    }
                    continue;
                }
                if (key == SDLK_RIGHT || key == SDLK_d) {
                    if (m_game) {
                        m_game->handleInput(PlayerCommand {Direction::Right, DrawMode::None});
                    }
                    continue;
                }
                if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) {
                    if (m_game) {
                        if (view.nameEntry.cursorIndex < 2) {
                            m_game->handleInput(PlayerCommand {Direction::Right, DrawMode::None});
                        } else {
                            m_game->confirmInitials();
                        }
                    }
                    continue;
                }
                if ((key >= SDLK_a && key <= SDLK_z) || (key >= SDLK_0 && key <= SDLK_9)) {
                    if (m_game) {
                        m_game->inputInitialsChar(static_cast<char>(key));
                    }
                    continue;
                }
            } else if (view.state == GameState::HallOfFame || view.state == GameState::GameOver) {
                if (key == SDLK_r || key == SDLK_SPACE || key == SDLK_RETURN || key == SDLK_KP_ENTER) {
                    if (m_game) {
                        m_game->reset();
                        setDelayMs(m_game->getCurrentDelayMs());
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
            case SDLK_c:
            case SDLK_F2:
                toggleCrt();
                break;
            case SDLK_m:
            case SDLK_F3:
                toggleAudio();
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

            m_audio.update(m_game->getView(), m_delayMs);

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
