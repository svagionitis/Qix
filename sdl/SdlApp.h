#ifndef QIX_SDL_APP_H
#define QIX_SDL_APP_H

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
    /// @brief Construct SdlApp with a game instance and initial simulation delay.
    /// @param[in] game Unique pointer to IQixGame engine instance.
    /// @param[in] delayMs Initial tick delay in milliseconds.
    explicit SdlApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs = SpeedConfig::DefaultDelayMs) noexcept;
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

private:
    std::unique_ptr<IQixGame> m_game;
    SdlRenderer m_renderer {};
    PlayerCommand m_currentCmd {};
    std::uint32_t m_delayMs {SpeedConfig::DefaultDelayMs};
    bool m_sdlInitialized {false};

    void processEvents(bool& running) noexcept;
};

} // namespace qix::sdl

#endif // QIX_SDL_APP_H
