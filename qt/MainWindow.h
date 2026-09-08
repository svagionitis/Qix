#pragma once
#include "IQixGame.h"
#include "QixCanvas.h"
#include "SpeedConfig.h"
#include <QMainWindow>
#include <QTimer>
#include <memory>

namespace qix::qt {

/// @class MainWindow
/// @brief Main desktop window managing game loop pacing and keyboard input routing.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::unique_ptr<IQixGame> game, std::uint32_t delayMs = SpeedConfig::DefaultDelayMs,
        bool crtEnabled = false, QWidget* parent = nullptr);
    ~MainWindow() override = default;

    /// @brief Get the current tick delay in milliseconds.
    /// @return Current simulation delay in milliseconds.
    [[nodiscard]] std::uint32_t getDelayMs() const noexcept;

    /// @brief Set the simulation delay in milliseconds, reconfiguring pacing timer.
    /// @param[in] delayMs Desired tick delay in milliseconds.
    void setDelayMs(std::uint32_t delayMs) noexcept;

    /// @brief Increase game speed by reducing tick delay.
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

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onTick();

private:
    std::unique_ptr<IQixGame> m_game;
    QixCanvas* m_canvas {nullptr};
    QAction* m_crtAction {nullptr};
    QTimer m_timer;
    PlayerCommand m_currentCmd {};
    std::uint32_t m_delayMs {SpeedConfig::DefaultDelayMs};
};

} // namespace qix::qt
