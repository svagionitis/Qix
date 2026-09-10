#pragma once
#include "ArcadeAudio.h"
#include "IQixGame.h"
#include "RaylibRenderer.h"
#include "ReplaySystem.h"
#include "SpeedConfig.h"
#include <cstdint>
#include <memory>
#include <string>

namespace qix::raylib {

/// @class RaylibApp
/// @brief Raylib application controller coordinating input dispatch, pacing timer, and rendering.
class RaylibApp {
public:
    /// @brief Construct RaylibApp with a game instance, initial simulation delay, CRT filter state, audio state, and
    /// palette.
    /// @param[in] game Unique pointer to IQixGame engine instance.
    /// @param[in] delayMs Initial tick delay in milliseconds.
    /// @param[in] crtEnabled Whether CRT scanlines & phosphor glow filter is initially active.
    /// @param[in] audioEnabled Whether procedural chiptune audio is initially active (default: false).
    /// @param[in] palette Initial visual color palette theme (default: PaletteId::Classic).
    explicit RaylibApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs = SpeedConfig::DefaultDelayMs,
        bool crtEnabled = false, bool audioEnabled = false, PaletteId palette = PaletteId::Classic,
        bool artEnabled = true, int artScene = -1) noexcept;
    ~RaylibApp();

    // Non-copyable and non-movable
    RaylibApp(const RaylibApp&) = delete;
    RaylibApp& operator=(const RaylibApp&) = delete;
    RaylibApp(RaylibApp&&) = delete;
    RaylibApp& operator=(RaylibApp&&) = delete;

    /// @brief Initialize Raylib display window.
    /// @param[in] title Window title.
    /// @param[in] width Initial window width in pixels.
    /// @param[in] height Initial window height in pixels.
    /// @return True on success, false on error.
    [[nodiscard]] bool init(
        const std::string& title = "Qix Arcade (Raylib)", int width = 960, int height = 720) noexcept;

    /// @brief Enter the main simulation and rendering loop.
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
    /// @param[in] enabled True to enable art reveal mode.
    void setArtEnabled(bool enabled) noexcept;

    /// @brief Check whether background art reveal mode is enabled.
    /// @return True if art reveal mode is active.
    [[nodiscard]] bool isArtEnabled() const noexcept;

    /// @brief Toggle background art reveal mode on/off at runtime.
    void toggleArt() noexcept;

    /// @brief Force a specific artwork scene, or -1 for auto level-based.
    /// @param[in] scene Scene index (0..3) or -1 for auto.
    void setArtScene(int scene) noexcept;

    /// @brief Access internal audio synthesizer.
    /// @return Reference to ArcadeAudio.
    [[nodiscard]] ArcadeAudio& getAudio() noexcept;

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
    RaylibRenderer m_renderer {};
    ArcadeAudio m_audio {};
    PlayerCommand m_currentCmd {};
    std::uint32_t m_delayMs {SpeedConfig::DefaultDelayMs};
    AudioStream m_audioStream {};
    bool m_audioDeviceReady {false};
    ReplayRecorder m_recorder {};
    ReplayPlayer m_player {};
    std::string m_recordPath {};
    std::uint32_t m_simTick {0};
    bool m_replaying {false};
    double m_lastStepTime {0.0};

    void processInput() noexcept;
};

} // namespace qix::raylib
