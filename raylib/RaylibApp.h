#pragma once
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
    /// @brief Construct RaylibApp with a game instance, initial simulation delay, and CRT filter state.
    /// @param[in] game Unique pointer to IQixGame engine instance.
    /// @param[in] delayMs Initial tick delay in milliseconds.
    /// @param[in] crtEnabled Whether CRT scanlines & phosphor glow filter is initially active.
    explicit RaylibApp(std::unique_ptr<IQixGame> game, std::uint32_t delayMs = SpeedConfig::DefaultDelayMs,
        bool crtEnabled = false) noexcept;
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

    /// @brief Enable or disable CRT scanlines & phosphor glow filter.
    /// @param[in] enabled True to enable CRT filter.
    void setCrtEnabled(bool enabled) noexcept;

    /// @brief Check whether CRT filter is currently active.
    /// @return True if CRT filter is enabled.
    [[nodiscard]] bool isCrtEnabled() const noexcept;

    /// @brief Toggle CRT filter on/off at runtime.
    void toggleCrt() noexcept;

private:
    std::unique_ptr<IQixGame> m_game;
    RaylibRenderer m_renderer {};
    PlayerCommand m_currentCmd {};
    std::uint32_t m_delayMs {SpeedConfig::DefaultDelayMs};

    void processInput() noexcept;
};

} // namespace qix::raylib
