#ifndef QIX_RAYLIB_APP_H
#define QIX_RAYLIB_APP_H

#include "IQixGame.h"
#include "RaylibRenderer.h"
#include "SpeedConfig.h"
#include <cstdint>
#include <memory>
#include <string>

namespace qix::raylib {

/// @class RaylibApp
/// @brief Raylib application controller coordinating input dispatch, pacing timer, and rendering.
class RaylibApp {
public:
    /// @brief Construct RaylibApp with a game instance and initial simulation delay.
    /// @param[in] game Unique pointer to IQixGame engine instance.
    /// @param[in] delayMs Initial tick delay in milliseconds.
    explicit RaylibApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs = SpeedConfig::DefaultDelayMs) noexcept;
    ~RaylibApp() = default;

    // Non-copyable
    RaylibApp(const RaylibApp&) = delete;
    RaylibApp& operator=(const RaylibApp&) = delete;

    // Movable
    RaylibApp(RaylibApp&&) noexcept = default;
    RaylibApp& operator=(RaylibApp&&) noexcept = default;

    /// @brief Initialize Raylib display window.
    /// @param[in] title Window title.
    /// @param[in] width Initial window width in pixels.
    /// @param[in] height Initial window height in pixels.
    /// @return True on success, false on error.
    [[nodiscard]] bool init(
        const std::string& title = "Qix Arcade (Raylib)", int width = 960, int height = 720) noexcept;

    /// @brief Enter the main simulation and rendering loop.
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
    RaylibRenderer m_renderer {};
    PlayerCommand m_currentCmd {};
    std::uint32_t m_delayMs {SpeedConfig::DefaultDelayMs};

    void processInput() noexcept;
};

} // namespace qix::raylib

#endif // QIX_RAYLIB_APP_H
