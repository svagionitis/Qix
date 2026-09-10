#include "MainWindow.h"
#include <QActionGroup>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QMenuBar>
#include <QScreen>
#include <cmath>

#if defined(QIX_QT_HAS_MULTIMEDIA)
#include <QAudioFormat>
#include <QIODevice>
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
#include <QAudioSink>
#include <QMediaDevices>
#else
#include <QAudioDeviceInfo>
#include <QAudioOutput>
#endif

namespace {
class AudioStreamDevice : public QIODevice {
public:
    explicit AudioStreamDevice(qix::ArcadeAudio& audio, QObject* parent = nullptr)
        : QIODevice {parent}
        , m_audio {audio}
    {
        open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    }

    bool isSequential() const override
    {
        return true;
    }

    qint64 bytesAvailable() const override
    {
        return 4096 + QIODevice::bytesAvailable();
    }

protected:
    qint64 readData(char* data, qint64 maxlen) override
    {
        const auto samplesToGenerate = static_cast<std::size_t>(maxlen / sizeof(std::int16_t));
        if (samplesToGenerate > 0 && data != nullptr) {
            m_audio.generateSamples(reinterpret_cast<std::int16_t*>(data), samplesToGenerate);
            return static_cast<qint64>(samplesToGenerate * sizeof(std::int16_t));
        }
        return 0;
    }

    qint64 writeData(const char* /*data*/, qint64 /*len*/) override
    {
        return 0;
    }

private:
    qix::ArcadeAudio& m_audio;
};
} // namespace
#endif

namespace qix::qt {

MainWindow::MainWindow(std::unique_ptr<IQixGame> game, std::uint32_t delayMs, bool crtEnabled, bool audioEnabled,
    PaletteId palette, bool artEnabled, int artScene, QWidget* parent)
    : QMainWindow {parent}
    , m_game {std::move(game)}
    , m_delayMs {SpeedConfig::clampDelay(delayMs)}
{
    setWindowTitle("Qix Arcade (C++17)");
    resize(960, 720);

    m_canvas = new QixCanvas(this);
    m_canvas->setDelayMs(m_delayMs);
    m_canvas->setCrtEnabled(crtEnabled);
    m_canvas->setPalette(palette);
    m_canvas->setArtEnabled(artEnabled);
    m_canvas->setArtScene(artScene);
    setCentralWidget(m_canvas);

    // Menu Bar with Game -> QuickSave, QuickLoad and View -> CRT Filter, Sound, Art Reveal, and Theme
    auto* gameMenu = menuBar()->addMenu(tr("&Game"));
    auto* quickSaveAction = gameMenu->addAction(tr("&QuickSave (F5)"), [this]() {
        if (m_game) {
            (void)m_game->quickSave();
        }
    });
    quickSaveAction->setShortcut(QKeySequence(Qt::Key_F5));

    auto* quickLoadAction = gameMenu->addAction(tr("Quick&Load (F9)"), [this]() {
        if (m_game && m_game->quickLoad()) {
            setDelayMs(m_game->getCurrentDelayMs());
            m_canvas->resetInterpolation();
            m_canvas->updateView(m_game->getView());
        }
    });
    quickLoadAction->setShortcut(QKeySequence(Qt::Key_F9));

    auto* viewMenu = menuBar()->addMenu(tr("&View"));
    m_crtAction = viewMenu->addAction(tr("&CRT Filter (Scanlines && Glow)"), this, &MainWindow::toggleCrt);
    m_crtAction->setCheckable(true);
    m_crtAction->setChecked(crtEnabled);
    m_crtAction->setShortcut(QKeySequence(Qt::Key_F2));

    m_audioAction = viewMenu->addAction(tr("&Sound (M / F3)"), this, &MainWindow::toggleAudio);
    m_audioAction->setCheckable(true);
    m_audioAction->setChecked(audioEnabled);
    m_audioAction->setShortcut(QKeySequence(Qt::Key_F3));

    m_artAction = viewMenu->addAction(tr("Background &Art Reveal (V)"), this, &MainWindow::toggleArt);
    m_artAction->setCheckable(true);
    m_artAction->setChecked(artEnabled);
    m_artAction->setShortcut(QKeySequence(Qt::Key_V));

    auto* themeMenu = viewMenu->addMenu(tr("&Theme"));
    auto* themeGroup = new QActionGroup(this);

    auto addThemeAction = [this, themeMenu, themeGroup](const QString& name, PaletteId id, bool checked) {
        auto* action = themeMenu->addAction(name, [this, id]() { setPalette(id); });
        action->setCheckable(true);
        action->setChecked(checked);
        themeGroup->addAction(action);
        return action;
    };

    m_classicThemeAction = addThemeAction(tr("&Classic 1981"), PaletteId::Classic, palette == PaletteId::Classic);
    m_synthwaveThemeAction
        = addThemeAction(tr("&Synthwave Neon"), PaletteId::Synthwave, palette == PaletteId::Synthwave);
    m_amberThemeAction = addThemeAction(tr("&P3 Amber CRT"), PaletteId::Amber, palette == PaletteId::Amber);
    m_greenThemeAction = addThemeAction(tr("&P1 Green CRT"), PaletteId::Green, palette == PaletteId::Green);

    themeMenu->addSeparator();
    auto* cycleThemeAction = themeMenu->addAction(tr("Cycle &Theme (F4)"), this, &MainWindow::cyclePalette);
    cycleThemeAction->setShortcut(QKeySequence(Qt::Key_F4));

    m_audio.setMuted(!audioEnabled);
    initAudio(audioEnabled);

    // Dynamic simulation and rendering loop
    connect(&m_simTimer, &QTimer::timeout, this, &MainWindow::onSimTick);
    m_simTimer.start(static_cast<int>(m_delayMs));

    connect(&m_renderTimer, &QTimer::timeout, this, &MainWindow::onRenderTick);
    int renderIntervalMs = 7;
    if (const auto* scr = QGuiApplication::primaryScreen()) {
        const auto rate = scr->refreshRate();
        if (rate > 10.0) {
            renderIntervalMs = std::max(1, static_cast<int>(std::round(1000.0 / rate)));
        }
    }
    m_renderTimer.start(renderIntervalMs);
}

MainWindow::~MainWindow()
{
    m_simTimer.stop();
    m_renderTimer.stop();
    if (m_recorder.isRecording() && !m_recordPath.empty() && m_game) {
        m_recorder.finish(m_game->getView().stats.score, m_simTick);
        static_cast<void>(m_recorder.saveToFile(m_recordPath));
    }
#if defined(QIX_QT_HAS_MULTIMEDIA)
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (m_audioSink) {
        m_audioSink->stop();
    }
#else
    if (m_audioOutput) {
        m_audioOutput->stop();
    }
#endif
    if (m_audioStreamDevice) {
        m_audioStreamDevice->close();
    }
#endif
}

void MainWindow::initAudio(bool /*audioEnabled*/)
{
#if defined(QIX_QT_HAS_MULTIMEDIA)
    QAudioFormat format;
    format.setSampleRate(static_cast<int>(ArcadeAudio::SampleRate));
    format.setChannelCount(1);
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    format.setSampleFormat(QAudioFormat::Int16);
    const auto device = QMediaDevices::defaultAudioOutput();
    if (!device.isNull()) {
        m_audioStreamDevice = std::make_unique<AudioStreamDevice>(m_audio, this);
        m_audioSink = std::make_unique<QAudioSink>(device, format, this);
        m_audioSink->start(m_audioStreamDevice.get());
    }
#else
    format.setSampleSize(16);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    const auto info = QAudioDeviceInfo::defaultOutputDevice();
    if (!info.isNull()) {
        m_audioStreamDevice = std::make_unique<AudioStreamDevice>(m_audio, this);
        m_audioOutput = std::make_unique<QAudioOutput>(info, format, this);
        m_audioOutput->start(m_audioStreamDevice.get());
    }
#endif
#endif
}

std::uint32_t MainWindow::getDelayMs() const noexcept
{
    return m_delayMs;
}

void MainWindow::setDelayMs(std::uint32_t delayMs) noexcept
{
    m_delayMs = SpeedConfig::clampDelay(delayMs);
    if (m_simTimer.isActive()) {
        m_simTimer.setInterval(static_cast<int>(m_delayMs));
    }
    if (m_canvas != nullptr) {
        m_canvas->setDelayMs(m_delayMs);
    }
}

void MainWindow::speedUp() noexcept
{
    if (m_game) {
        m_game->setBaseDelayMs(SpeedConfig::speedUp(m_game->getBaseDelayMs()));
        setDelayMs(m_game->getCurrentDelayMs());
    } else {
        setDelayMs(SpeedConfig::speedUp(m_delayMs));
    }
}

void MainWindow::speedDown() noexcept
{
    if (m_game) {
        m_game->setBaseDelayMs(SpeedConfig::speedDown(m_game->getBaseDelayMs()));
        setDelayMs(m_game->getCurrentDelayMs());
    } else {
        setDelayMs(SpeedConfig::speedDown(m_delayMs));
    }
}

void MainWindow::setCrtEnabled(bool enabled) noexcept
{
    if (m_canvas != nullptr) {
        m_canvas->setCrtEnabled(enabled);
    }
    if (m_crtAction != nullptr) {
        m_crtAction->setChecked(enabled);
    }
}

bool MainWindow::isCrtEnabled() const noexcept
{
    return m_canvas != nullptr ? m_canvas->isCrtEnabled() : false;
}

void MainWindow::toggleCrt() noexcept
{
    if (m_canvas != nullptr) {
        m_canvas->toggleCrt();
        if (m_crtAction != nullptr) {
            m_crtAction->setChecked(m_canvas->isCrtEnabled());
        }
    }
}

void MainWindow::setAudioEnabled(bool enabled) noexcept
{
    m_audio.setMuted(!enabled);
    if (m_audioAction != nullptr) {
        m_audioAction->setChecked(enabled);
    }
}

bool MainWindow::isAudioEnabled() const noexcept
{
    return !m_audio.isMuted();
}

void MainWindow::toggleAudio() noexcept
{
    m_audio.toggleMute();
    if (m_audioAction != nullptr) {
        m_audioAction->setChecked(!m_audio.isMuted());
    }
}

void MainWindow::setPalette(PaletteId id) noexcept
{
    if (m_canvas != nullptr) {
        m_canvas->setPalette(id);
    }
    if (m_classicThemeAction != nullptr) {
        m_classicThemeAction->setChecked(id == PaletteId::Classic);
    }
    if (m_synthwaveThemeAction != nullptr) {
        m_synthwaveThemeAction->setChecked(id == PaletteId::Synthwave);
    }
    if (m_amberThemeAction != nullptr) {
        m_amberThemeAction->setChecked(id == PaletteId::Amber);
    }
    if (m_greenThemeAction != nullptr) {
        m_greenThemeAction->setChecked(id == PaletteId::Green);
    }
}

PaletteId MainWindow::getPalette() const noexcept
{
    return (m_canvas != nullptr) ? m_canvas->getPalette() : PaletteId::Classic;
}

void MainWindow::cyclePalette() noexcept
{
    const auto nextId = ColorPalette::next(getPalette());
    setPalette(nextId);
}

void MainWindow::setArtEnabled(bool enabled) noexcept
{
    if (m_canvas != nullptr) {
        m_canvas->setArtEnabled(enabled);
    }
    if (m_artAction != nullptr) {
        m_artAction->setChecked(enabled);
    }
}

bool MainWindow::isArtEnabled() const noexcept
{
    return (m_canvas != nullptr) ? m_canvas->isArtEnabled() : true;
}

void MainWindow::toggleArt() noexcept
{
    if (m_canvas != nullptr) {
        m_canvas->toggleArt();
        if (m_artAction != nullptr) {
            m_artAction->setChecked(m_canvas->isArtEnabled());
        }
    }
}

void MainWindow::setArtScene(int scene) noexcept
{
    if (m_canvas != nullptr) {
        m_canvas->setArtScene(scene);
    }
}

void MainWindow::setRecordPath(const std::string& recordPath) noexcept
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
        setWindowTitle(windowTitle() + " [REC]");
    }
}

void MainWindow::setReplayPlayer(ReplayPlayer player) noexcept
{
    m_player = std::move(player);
    m_replaying = m_player.isLoaded();
    m_simTick = 0;
    if (m_replaying) {
        setWindowTitle(windowTitle() + " [REPLAY]");
    }
}

bool MainWindow::isReplaying() const noexcept
{
    return m_replaying;
}

bool MainWindow::isRecording() const noexcept
{
    return m_recorder.isRecording();
}

void MainWindow::onSimTick()
{
    if (!m_game) {
        return;
    }

    m_canvas->onSimulationTick(m_game->getView());

    if (m_replaying) {
        m_currentCmd = m_player.getCommandForTick(m_simTick);
        if (m_game->getView().state == GameState::LevelComplete) {
            if (m_currentCmd.drawMode != DrawMode::None || m_currentCmd.direction != Direction::None) {
                m_game->nextLevel();
                setDelayMs(m_game->getCurrentDelayMs());
                m_canvas->resetInterpolation();
            }
        }
    } else if (m_recorder.isRecording()) {
        m_recorder.recordTick(m_simTick, m_currentCmd);
    }

    m_game->handleInput(m_currentCmd);
    m_game->step(m_delayMs);
    ++m_simTick;

    m_lastStepTime = std::chrono::steady_clock::now();

    m_audio.update(m_game->getView(), m_delayMs);

    m_canvas->updateView(m_game->getView(), 0.0f);

    // Clear direction after step
    m_currentCmd.direction = Direction::None;
}

void MainWindow::onRenderTick()
{
    if (!m_game) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration_cast<std::chrono::duration<float>>(now - m_lastStepTime).count();
    const float interval = static_cast<float>(m_delayMs) / 1000.0f;
    const float alpha = MotionInterpolator::calculateAlpha(elapsed, interval);

    m_canvas->setInterpolationAlpha(alpha);
    m_canvas->update();
}

void MainWindow::keyPressEvent(QKeyEvent* event)
{
    const auto view = m_game->getView();

    if (view.state == GameState::Attract) {
        m_game->exitAttractMode();
        m_canvas->updateView(m_game->getView());
        return;
    } else if (event->key() == Qt::Key_F1) {
        m_game->startAttractMode();
        m_canvas->updateView(m_game->getView());
        return;
    }

    // Restart or advance level on state screens
    if (view.state == GameState::LevelComplete) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Return) {
            m_game->nextLevel();
            setDelayMs(m_game->getCurrentDelayMs());
            m_canvas->resetInterpolation();
            m_canvas->updateView(m_game->getView());
            return;
        }
    } else if (view.state == GameState::NameEntry) {
        if (event->key() == Qt::Key_Up || event->key() == Qt::Key_W) {
            m_game->handleInput(PlayerCommand {Direction::Up, DrawMode::None});
            m_canvas->updateView(m_game->getView());
            return;
        }
        if (event->key() == Qt::Key_Down || event->key() == Qt::Key_S) {
            m_game->handleInput(PlayerCommand {Direction::Down, DrawMode::None});
            m_canvas->updateView(m_game->getView());
            return;
        }
        if (event->key() == Qt::Key_Left || event->key() == Qt::Key_A) {
            m_game->handleInput(PlayerCommand {Direction::Left, DrawMode::None});
            m_canvas->updateView(m_game->getView());
            return;
        }
        if (event->key() == Qt::Key_Right || event->key() == Qt::Key_D) {
            m_game->handleInput(PlayerCommand {Direction::Right, DrawMode::None});
            m_canvas->updateView(m_game->getView());
            return;
        }
        if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter || event->key() == Qt::Key_Space) {
            if (view.nameEntry.cursorIndex < 2) {
                m_game->handleInput(PlayerCommand {Direction::Right, DrawMode::None});
            } else {
                m_game->confirmInitials();
            }
            m_canvas->updateView(m_game->getView());
            return;
        }
        const QString text = event->text();
        if (!text.isEmpty()) {
            const QChar ch = text.at(0);
            if (ch.isLetterOrNumber()) {
                m_game->inputInitialsChar(static_cast<char>(ch.toUpper().toLatin1()));
                m_canvas->updateView(m_game->getView());
                return;
            }
        }
        return;
    } else if (view.state == GameState::HallOfFame || view.state == GameState::GameOver) {
        if (event->key() == Qt::Key_R || event->key() == Qt::Key_Space || event->key() == Qt::Key_Return
            || event->key() == Qt::Key_Enter) {
            m_game->reset();
            setDelayMs(m_game->getCurrentDelayMs());
            m_canvas->resetInterpolation();
            m_canvas->updateView(m_game->getView());
            return;
        }
    }

    if (!m_replaying) {
        switch (event->key()) {
        case Qt::Key_Up:
        case Qt::Key_W:
            m_currentCmd.direction = Direction::Up;
            break;
        case Qt::Key_Down:
        case Qt::Key_S:
            m_currentCmd.direction = Direction::Down;
            break;
        case Qt::Key_Left:
        case Qt::Key_A:
            m_currentCmd.direction = Direction::Left;
            break;
        case Qt::Key_Right:
        case Qt::Key_D:
            m_currentCmd.direction = Direction::Right;
            break;
        case Qt::Key_Space:
        case Qt::Key_Control:
            m_currentCmd.drawMode = DrawMode::Slow;
            break;
        case Qt::Key_Shift:
        case Qt::Key_F:
            m_currentCmd.drawMode = DrawMode::Fast;
            break;
        default:
            break;
        }
    }

    switch (event->key()) {
    case Qt::Key_Minus:
    case Qt::Key_BracketLeft:
    case Qt::Key_Underscore:
        speedDown();
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
    case Qt::Key_BracketRight:
        speedUp();
        break;
    case Qt::Key_R:
        if (m_game) {
            m_game->reset();
            m_canvas->resetInterpolation();
            m_canvas->updateView(m_game->getView());
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
        break;
    case Qt::Key_C:
    case Qt::Key_F2:
        toggleCrt();
        break;
    case Qt::Key_M:
    case Qt::Key_F3:
        toggleAudio();
        break;
    case Qt::Key_P:
    case Qt::Key_Pause:
        if (m_game) {
            m_game->togglePause();
            m_canvas->updateView(m_game->getView());
        }
        break;
    case Qt::Key_F4:
        cyclePalette();
        break;
    case Qt::Key_V:
        toggleArt();
        break;
    case Qt::Key_F5:
        if (m_game) {
            (void)m_game->quickSave();
        }
        break;
    case Qt::Key_F9:
        if (m_game && m_game->quickLoad()) {
            setDelayMs(m_game->getCurrentDelayMs());
            m_canvas->updateView(m_game->getView());
        }
        break;
    case Qt::Key_Escape:
        close();
        break;
    default:
        QMainWindow::keyPressEvent(event);
        break;
    }
}

void MainWindow::keyReleaseEvent(QKeyEvent* event)
{
    if (!m_replaying) {
        if (event->key() == Qt::Key_Space || event->key() == Qt::Key_Control || event->key() == Qt::Key_Shift
            || event->key() == Qt::Key_F) {
            m_currentCmd.drawMode = DrawMode::None;
        }
    }

    QMainWindow::keyReleaseEvent(event);
}

} // namespace qix::qt
