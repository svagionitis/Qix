#pragma once
#include "ArcadeAudio.h"
#include "IQixGame.h"
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
    /// @brief Construct SdlApp with a game instance, initial simulation delay, CRT filter state, audio state, and
    /// palette.
    /// @param[in] game Unique pointer to IQixGame engine instance.
    /// @param[in] delayMs Initial tick delay in milliseconds.
    /// @param[in] crtEnabled Whether CRT scanlines & phosphor glow filter is initially active.
    /// @param[in] audioEnabled Whether procedural chiptune audio is initially active (default: false).
    /// @param[in] palette Initial visual color palette theme (default: PaletteId::Classic).
    explicit SdlApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs = SpeedConfig::DefaultDelayMs,
        bool crtEnabled = false, bool audioEnabled = false, PaletteId palette = PaletteId::Classic) noexcept;
    ~SdlApp();

    // Non-copyable
    SdlApp(const SdlApp&) = delete;
    SdlApp& operator=(const SdlApp&) = delete;

    // Movable
    SdlApp(SdlApp&&) noexcept = default;
    SdlApp& operator=(SdlApp&&) noexcept = default;

    /// @brief Initialize SDL subsystems and display window.
    /// @param[in] title Window title.
    /// @param[in] width Initial window width in pixels.
    /// @param[in] height Initial window height in pixels.
    /// @return True on success, false on error.
    [[nodiscard]] bool init(const std::string& title = "Qix Arcade (SDL2)", int width = 960, int height = 720) noexcept;

    /// @brief Enter the main application simulation and rendering loop.
    void run() noexcept;

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

private:
    std::unique_ptr<IQixGame> m_game;
    SdlRenderer m_renderer {};
    ArcadeAudio m_audio {};
    PlayerCommand m_currentCmd {};
    std::uint32_t m_delayMs {SpeedConfig::DefaultDelayMs};
    bool m_sdlInitialized {false};
    SDL_AudioDeviceID m_audioDevice {0};

    void processEvents(bool& running) noexcept;
    static void sdlAudioCallback(void* userdata, Uint8* stream, int len) noexcept;
};

} // namespace qix::sdl
