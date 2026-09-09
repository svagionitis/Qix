#pragma once
#include "ArcadeAudio.h"
#include "IQixGame.h"
#include "QixCanvas.h"
#include "ReplaySystem.h"
#include "SpeedConfig.h"
#include <QMainWindow>
#include <QTimer>
#include <memory>

#if defined(QIX_QT_HAS_MULTIMEDIA)
class QIODevice;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
class QAudioSink;
#else
class QAudioOutput;
#endif
#endif

namespace qix::qt {

/// @class MainWindow
/// @brief Main desktop window managing game loop pacing and keyboard input routing.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(std::unique_ptr<IQixGame> game, std::uint32_t delayMs = SpeedConfig::DefaultDelayMs,
        bool crtEnabled = false, bool audioEnabled = false, PaletteId palette = PaletteId::Classic,
        bool artEnabled = true, int artScene = -1, QWidget* parent = nullptr);
    ~MainWindow() override;

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

protected:
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private slots:
    void onTick();

private:
    std::unique_ptr<IQixGame> m_game;
    QixCanvas* m_canvas {nullptr};
    QAction* m_crtAction {nullptr};
    QAction* m_audioAction {nullptr};
    QAction* m_artAction {nullptr};
    QAction* m_classicThemeAction {nullptr};
    QAction* m_synthwaveThemeAction {nullptr};
    QAction* m_amberThemeAction {nullptr};
    QAction* m_greenThemeAction {nullptr};
    QTimer m_timer;
    PlayerCommand m_currentCmd {};
    std::uint32_t m_delayMs {SpeedConfig::DefaultDelayMs};
    ArcadeAudio m_audio {};
    ReplayRecorder m_recorder {};
    ReplayPlayer m_player {};
    std::string m_recordPath {};
    std::uint32_t m_simTick {0};
    bool m_replaying {false};

#if defined(QIX_QT_HAS_MULTIMEDIA)
    std::unique_ptr<QIODevice> m_audioStreamDevice;
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    std::unique_ptr<QAudioSink> m_audioSink;
#else
    std::unique_ptr<QAudioOutput> m_audioOutput;
#endif
#endif

    void initAudio(bool audioEnabled);
};

} // namespace qix::qt
