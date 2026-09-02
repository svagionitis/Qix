#ifndef QIX_GUI_MAIN_WINDOW_H
#define QIX_GUI_MAIN_WINDOW_H

#include "IQixGame.h"
#include "QixCanvas.h"
#include "SpeedConfig.h"
#include <QMainWindow>
#include <QTimer>
#include <memory>

namespace qix::gui {

/// @class MainWindow
/// @brief Main desktop window managing game loop pacing and keyboard input routing.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(
        std::unique_ptr<IQixGame> game, std::uint32_t delayMs = SpeedConfig::DefaultDelayMs, QWidget* parent = nullptr);
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

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onTick();

private:
    std::unique_ptr<IQixGame> m_game;
    QixCanvas* m_canvas {nullptr};
    QTimer m_timer;
    PlayerCommand m_currentCmd {};
    std::uint32_t m_delayMs {SpeedConfig::DefaultDelayMs};
};

} // namespace qix::gui

#endif // QIX_GUI_MAIN_WINDOW_H
