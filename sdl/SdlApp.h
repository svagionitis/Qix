#pragma once
#include "ArcadeAudio.h"
#include "IQixGame.h"
#include "ReplaySystem.h"
#include "SdlRenderer.h"
#include "SpeedConfig.h"
#include <SDL.h>
#include <cstdint>
#include <memory>
#include <string>

namespace qix::sdl {

/// @class SdlApp
/// @brief Desktop SDL2 application controller coordinating event routing, simulation pacing, and rendering.
class SdlApp {
public:
    /// @brief Construct SdlApp with a game instance, initial simulation delay, CRT filter state, audio state,
    /// palette, and background art reveal settings.
    /// @param[in] game Unique pointer to IQixGame engine instance.
    /// @param[in] delayMs Initial tick delay in milliseconds.
    /// @param[in] crtEnabled Whether CRT scanlines & phosphor glow filter is initially active.
    /// @param[in] audioEnabled Whether procedural chiptune audio is initially active (default: false).
    /// @param[in] palette Initial visual color palette theme (default: PaletteId::Classic).
    /// @param[in] artEnabled Whether background art reveal mode is active (default: true).
    /// @param[in] artScene Initial art scene index or -1 for level-based automatic progression (default: -1).
    explicit SdlApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs = SpeedConfig::DefaultDelayMs,
        bool crtEnabled = false, bool audioEnabled = false, PaletteId palette = PaletteId::Classic,
        bool artEnabled = true, int artScene = -1) noexcept;
    ~SdlApp();

    // Non-copyable and non-movable
    SdlApp(const SdlApp&) = delete;
    SdlApp& operator=(const SdlApp&) = delete;
    SdlApp(SdlApp&&) = delete;
    SdlApp& operator=(SdlApp&&) = delete;

    /// @brief Initialize SDL subsystems and display window.
    /// @param[in] title Window title.
    /// @param[in] width Initial window width in pixels.
    /// @param[in] height Initial window height in pixels.
    /// @return True on success, false on error.
    [[nodiscard]] bool init(const std::string& title = "Qix Arcade (SDL2)", int width = 960, int height = 720) noexcept;

    /// @brief Enter the main application simulation and rendering loop.
    void run() noexcept;

    /// @brief Execute a single frame tick consisting of input polling, simulation pacing, and rendering.
    /// @details Polled repeatedly by desktop run() or invoked per animation frame by Emscripten under WebAssembly.
    /// @return True if the application should continue running, false if an exit condition was encountered.
    [[nodiscard]] bool tick() noexcept;

    /// @brief Get current simulation delay in milliseconds.
    /// @return Current tick delay.
    [[nodiscard]] std::uint32_t getDelayMs() const noexcept;

    /// @brief Set simulation delay in milliseconds, clamped to allowable bounds.
    /// @param[in] delayMs New tick delay in milliseconds.
    void setDelayMs(std::uint32_t delayMs) noexcept;

    /// @brief Increase game speed by decreasing tick delay.
    void speedUp() noexcept;

    /// @brief Decrease game speed by increasing tick delay.
    void speedDown() noexcept;

    /// @brief Enable or disable CRT scanlines & phosphor glow filter.
    /// @param[in] enabled True to enable CRT filter.
    void setCrtEnabled(bool enabled) noexcept;

    /// @brief Check whether CRT filter is currently active.
    /// @return True if CRT filter is enabled.
    [[nodiscard]] bool isCrtEnabled() const noexcept;

    /// @brief Toggle CRT filter on/off at runtime.
    void toggleCrt() noexcept;

    /// @brief Enable or disable procedural arcade sound synthesis.
    /// @param[in] enabled True to unmute audio, false to mute.
    void setAudioEnabled(bool enabled) noexcept;

    /// @brief Check whether procedural sound is currently unmuted.
    /// @return True if sound is enabled.
    [[nodiscard]] bool isAudioEnabled() const noexcept;

    /// @brief Toggle procedural sound on/off at runtime.
    void toggleAudio() noexcept;

    /// @brief Set active visual color palette theme.
    /// @param[in] id Palette identifier.
    void setPalette(PaletteId id) noexcept;

    /// @brief Get active visual color palette theme identifier.
    /// @return Active PaletteId.
    [[nodiscard]] PaletteId getPalette() const noexcept;

    /// @brief Cycle to next color palette theme.
    void cyclePalette() noexcept;

    /// @brief Enable or disable background art reveal mode.
    /// @param[in] enabled True to reveal background artwork under claimed territory.
    void setArtEnabled(bool enabled) noexcept;

    /// @brief Check whether background art reveal mode is currently active.
    /// @return True if background art reveal is enabled.
    [[nodiscard]] bool isArtEnabled() const noexcept;

    /// @brief Toggle background art reveal mode on/off at runtime.
    void toggleArt() noexcept;

    /// @brief Set active background art scene (-1 for automatic progression with game level).
    /// @param[in] scene Scene index or -1 for auto.
    void setArtScene(int scene) noexcept;

    /// @brief Enable input recording to the given file upon exit.
    /// @param[in] recordPath Target replay file path.
    void setRecordPath(const std::string& recordPath) noexcept;

    /// @brief Enable playback from a preloaded ReplayPlayer.
    /// @param[in] player Initialized ReplayPlayer.
    void setReplayPlayer(ReplayPlayer player) noexcept;

    /// @brief Check whether currently in replay playback mode.
    /// @return True if replaying.
    [[nodiscard]] bool isReplaying() const noexcept;

    /// @brief Check whether currently recording.
    /// @return True if recording.
    [[nodiscard]] bool isRecording() const noexcept;

private:
    std::unique_ptr<IQixGame> m_game;
    SdlRenderer m_renderer {};
    ArcadeAudio m_audio {};
    PlayerCommand m_currentCmd {};
    std::uint32_t m_delayMs {SpeedConfig::DefaultDelayMs};
    bool m_sdlInitialized {false};
    SDL_AudioDeviceID m_audioDevice {0};
    ReplayRecorder m_recorder {};
    ReplayPlayer m_player {};
    std::string m_recordPath {};
    std::uint32_t m_simTick {0};
    bool m_replaying {false};
    std::uint32_t m_lastStepTicks {0};
    double m_lastStepTimeSec {0.0};

    void processEvents(bool& running) noexcept;
    static void sdlAudioCallback(void* userdata, Uint8* stream, int len) noexcept;
};

} // namespace qix::sdl
