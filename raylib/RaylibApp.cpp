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
    PaletteId palette, bool artEnabled, int artScene) noexcept
    : m_game {std::move(game)}
    , m_delayMs {SpeedConfig::clampDelay(delayMs)}
{
    m_renderer.setCrtEnabled(crtEnabled);
    m_renderer.setPalette(palette);
    m_renderer.setArtEnabled(artEnabled);
    m_renderer.setArtScene(artScene);
    m_audio.setMuted(!audioEnabled);
}

RaylibApp::~RaylibApp()
{
    if (m_recorder.isRecording() && !m_recordPath.empty() && m_game) {
        m_recorder.finish(m_game->getView().stats.score, m_simTick);
        static_cast<void>(m_recorder.saveToFile(m_recordPath));
    }
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
    std::string finalTitle = title;
    if (m_replaying) {
        finalTitle += " [REPLAY]";
    } else if (m_recorder.isRecording()) {
        finalTitle += " [REC]";
    }

    const bool ok = m_renderer.init(finalTitle, width, height);
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

void RaylibApp::setArtEnabled(bool enabled) noexcept
{
    m_renderer.setArtEnabled(enabled);
}

bool RaylibApp::isArtEnabled() const noexcept
{
    return m_renderer.isArtEnabled();
}

void RaylibApp::toggleArt() noexcept
{
    m_renderer.toggleArt();
}

void RaylibApp::setArtScene(int scene) noexcept
{
    m_renderer.setArtScene(scene);
}

void RaylibApp::setRecordPath(const std::string& recordPath) noexcept
{
    m_recordPath = recordPath;
    if (!m_recordPath.empty() && m_game) {
        const auto& view = m_game->getView();
        ReplayHeader hdr {};
        hdr.mode = m_game->getGameMode();
        hdr.playfieldWidth = (view.playfield != nullptr) ? view.playfield->getWidth() : 80;
        hdr.playfieldHeight = (view.playfield != nullptr) ? view.playfield->getHeight() : 60;
        hdr.targetPercent = view.stats.targetPercent;
        hdr.baseDelayMs = m_game->getBaseDelayMs();
        m_recorder.start(hdr);
        m_simTick = 0;
    }
}

void RaylibApp::setReplayPlayer(ReplayPlayer player) noexcept
{
    m_player = std::move(player);
    m_replaying = m_player.isLoaded();
    m_simTick = 0;
}

bool RaylibApp::isReplaying() const noexcept
{
    return m_replaying;
}

bool RaylibApp::isRecording() const noexcept
{
    return m_recorder.isRecording();
}

void RaylibApp::processInput() noexcept
{
    const auto view = m_game ? m_game->getView() : GameView {};

    // State screen transitions
    if (view.state == GameState::Attract) {
        if (GetKeyPressed() > 0 || GetCharPressed() > 0) {
            if (m_game) {
                m_game->exitAttractMode();
            }
            return;
        }
    } else if (IsKeyPressed(KEY_F1)) {
        if (m_game) {
            m_game->startAttractMode();
        }
        return;
    }

    if (view.state == GameState::LevelComplete) {
        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
            if (m_game) {
                m_game->nextLevel();
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

    if (!m_replaying) {
        // Direction controls
        Direction dir {Direction::None};
        if (IsKeyDown(KEY_UP) || IsKeyDown(KEY_W)) {
            dir = Direction::Up;
        } else if (IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S)) {
            dir = Direction::Down;
        } else if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A)) {
            dir = Direction::Left;
        } else if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) {
            dir = Direction::Right;
        }

        // Two-button arcade draw mode controls
        DrawMode mode {DrawMode::None};
        if (IsKeyDown(KEY_SPACE) || IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL)) {
            mode = DrawMode::Slow;
        } else if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT) || IsKeyDown(KEY_F)) {
            mode = DrawMode::Fast;
        }

        m_currentCmd.direction = dir;
        m_currentCmd.drawMode = mode;
    }

    // Speed pacing runtime controls
    if (IsKeyPressed(KEY_EQUAL) || IsKeyPressed(KEY_KP_ADD) || IsKeyPressed(KEY_RIGHT_BRACKET)) {
        speedUp();
    }
    if (IsKeyPressed(KEY_MINUS) || IsKeyPressed(KEY_KP_SUBTRACT) || IsKeyPressed(KEY_LEFT_BRACKET)) {
        speedDown();
    }

    // Session reset
    if (IsKeyPressed(KEY_R)) {
        if (m_game) {
            m_game->reset();
            if (m_recorder.isRecording()) {
                ReplayHeader hdr {};
                hdr.mode = m_game->getGameMode();
                hdr.playfieldWidth = (view.playfield != nullptr) ? view.playfield->getWidth() : 80;
                hdr.playfieldHeight = (view.playfield != nullptr) ? view.playfield->getHeight() : 60;
                hdr.targetPercent = view.stats.targetPercent;
                hdr.baseDelayMs = m_game->getBaseDelayMs();
                m_recorder.start(hdr);
                m_simTick = 0;
            }
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

    // Toggle background art reveal mode
    if (IsKeyPressed(KEY_V)) {
        toggleArt();
    }

    // QuickSave / QuickLoad session
    if (IsKeyPressed(KEY_F5)) {
        if (m_game) {
            (void)m_game->quickSave();
        }
    }
    if (IsKeyPressed(KEY_F9)) {
        if (m_game) {
            if (m_game->quickLoad()) {
                m_delayMs = m_game->getCurrentDelayMs();
            }
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
                if (m_replaying) {
                    m_currentCmd = m_player.getCommandForTick(m_simTick);
                    if (m_game->getView().state == GameState::LevelComplete) {
                        if (m_currentCmd.drawMode != DrawMode::None || m_currentCmd.direction != Direction::None) {
                            m_game->nextLevel();
                        }
                    }
                } else if (m_recorder.isRecording()) {
                    m_recorder.recordTick(m_simTick, m_currentCmd);
                }

                m_game->handleInput(m_currentCmd);
                m_game->step(m_delayMs);
                ++m_simTick;

                m_audio.update(m_game->getView(), m_delayMs);
                m_currentCmd.direction = Direction::None;
            }
            lastStepTime = currentTime;
        }

        if (m_game) {
            m_renderer.render(m_game->getView(), m_delayMs);
        }
    }

    if (m_recorder.isRecording() && !m_recordPath.empty() && m_game) {
        m_recorder.finish(m_game->getView().stats.score, m_simTick);
        static_cast<void>(m_recorder.saveToFile(m_recordPath));
    }
}

} // namespace qix::raylib
