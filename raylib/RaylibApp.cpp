#include "RaylibApp.h"
#include <algorithm>

namespace qix::raylib {

static RaylibApp* s_currentApp {nullptr};

static void raylibAudioCallback(void* bufferData, unsigned int frames) noexcept
{
    if (s_currentApp != nullptr && bufferData != nullptr && frames > 0) {
        s_currentApp->getAudio().generateSamples(
            reinterpret_cast<std::int16_t*>(bufferData), static_cast<std::size_t>(frames));
    } else if (bufferData != nullptr && frames > 0) {
        std::fill_n(reinterpret_cast<std::int16_t*>(bufferData), frames, static_cast<std::int16_t>(0));
    }
}

RaylibApp::RaylibApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs, bool crtEnabled, bool audioEnabled,
    PaletteId palette) noexcept
    : m_game {std::move(game)}
    , m_delayMs {SpeedConfig::clampDelay(delayMs)}
{
    m_renderer.setCrtEnabled(crtEnabled);
    m_renderer.setPalette(palette);
    m_audio.setMuted(!audioEnabled);
}

RaylibApp::~RaylibApp()
{
    if (m_audioDeviceReady) {
        if (s_currentApp == this) {
            s_currentApp = nullptr;
        }
        UnloadAudioStream(m_audioStream);
        CloseAudioDevice();
        m_audioDeviceReady = false;
    }
}

bool RaylibApp::init(const std::string& title, int width, int height) noexcept
{
    const bool ok = m_renderer.init(title, width, height);
    if (!ok) {
        return false;
    }

    InitAudioDevice();
    if (IsAudioDeviceReady()) {
        m_audioDeviceReady = true;
        m_audioStream = LoadAudioStream(ArcadeAudio::SampleRate, 16, 1);
        s_currentApp = this;
        SetAudioStreamCallback(m_audioStream, raylibAudioCallback);
        PlayAudioStream(m_audioStream);
    }

    return true;
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
    if (m_game) {
        m_game->setBaseDelayMs(SpeedConfig::speedUp(m_game->getBaseDelayMs()));
        setDelayMs(m_game->getCurrentDelayMs());
    } else {
        setDelayMs(SpeedConfig::speedUp(m_delayMs));
    }
}

void RaylibApp::speedDown() noexcept
{
    if (m_game) {
        m_game->setBaseDelayMs(SpeedConfig::speedDown(m_game->getBaseDelayMs()));
        setDelayMs(m_game->getCurrentDelayMs());
    } else {
        setDelayMs(SpeedConfig::speedDown(m_delayMs));
    }
}

void RaylibApp::setCrtEnabled(bool enabled) noexcept
{
    m_renderer.setCrtEnabled(enabled);
}

bool RaylibApp::isCrtEnabled() const noexcept
{
    return m_renderer.isCrtEnabled();
}

void RaylibApp::toggleCrt() noexcept
{
    m_renderer.toggleCrt();
}

void RaylibApp::setAudioEnabled(bool enabled) noexcept
{
    m_audio.setMuted(!enabled);
}

bool RaylibApp::isAudioEnabled() const noexcept
{
    return !m_audio.isMuted();
}

void RaylibApp::toggleAudio() noexcept
{
    m_audio.toggleMute();
}

ArcadeAudio& RaylibApp::getAudio() noexcept
{
    return m_audio;
}

void RaylibApp::setPalette(PaletteId id) noexcept
{
    m_renderer.setPalette(id);
}

PaletteId RaylibApp::getPalette() const noexcept
{
    return m_renderer.getPalette();
}

void RaylibApp::cyclePalette() noexcept
{
    m_renderer.cyclePalette();
}

void RaylibApp::processInput() noexcept
{
    const auto view = m_game ? m_game->getView() : GameView {};

    // State screen transitions
    if (view.state == GameState::LevelComplete) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if (m_game) {
                m_game->nextLevel();
                m_delayMs = m_game->getCurrentDelayMs();
            }
            return;
        }
    } else if (view.state == GameState::NameEntry) {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            if (m_game) {
                m_game->handleInput(PlayerCommand {Direction::Up, DrawMode::None});
            }
            return;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            if (m_game) {
                m_game->handleInput(PlayerCommand {Direction::Down, DrawMode::None});
            }
            return;
        }
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            if (m_game) {
                m_game->handleInput(PlayerCommand {Direction::Left, DrawMode::None});
            }
            return;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            if (m_game) {
                m_game->handleInput(PlayerCommand {Direction::Right, DrawMode::None});
            }
            return;
        }
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (m_game) {
                if (view.nameEntry.cursorIndex < 2) {
                    m_game->handleInput(PlayerCommand {Direction::Right, DrawMode::None});
                } else {
                    m_game->confirmInitials();
                }
            }
            return;
        }

        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 'a' && key <= 'z') || (key >= 'A' && key <= 'Z') || (key >= '0' && key <= '9')) {
                if (m_game) {
                    m_game->inputInitialsChar(static_cast<char>(key));
                }
            }
            key = GetCharPressed();
        }
        return;
    } else if (view.state == GameState::HallOfFame || view.state == GameState::GameOver) {
        if (IsKeyPressed(KEY_R) || IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_KP_ENTER)) {
            if (m_game) {
                m_game->reset();
                m_delayMs = m_game->getCurrentDelayMs();
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

    // Toggle CRT scanlines & phosphor glow filter
    if (IsKeyPressed(KEY_C) || IsKeyPressed(KEY_F2)) {
        toggleCrt();
    }

    // Toggle procedural arcade audio
    if (IsKeyPressed(KEY_M) || IsKeyPressed(KEY_F3)) {
        toggleAudio();
    }

    // Cycle arcade color palette theme
    if (IsKeyPressed(KEY_F4)) {
        cyclePalette();
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
                m_audio.update(m_game->getView(), m_delayMs);
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
