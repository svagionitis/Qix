#pragma once
#include "ArcadeAudio.h"
#include <atomic>
#include <cstdint>
#include <thread>

namespace qix::tui {

/// @class TuiAudioStream
/// @brief Low-latency audio streaming backend for the terminal TUI client.
/// @details Streams 44.1 kHz 16-bit mono PCM synthesized by ArcadeAudio to the system sound card
/// using ALSA on Linux, or a zero-cost dummy fallback driver when audio hardware is unavailable.
class TuiAudioStream {
public:
    /// @brief Construct audio streamer bound to an ArcadeAudio synthesizer.
    /// @param[in] audio Reference to the ArcadeAudio synthesizer.
    explicit TuiAudioStream(ArcadeAudio& audio) noexcept;

    /// @brief Destructor stops playback thread and releases audio device handles.
    ~TuiAudioStream() noexcept;

    // Non-copyable, non-movable
    TuiAudioStream(const TuiAudioStream&) = delete;
    TuiAudioStream& operator=(const TuiAudioStream&) = delete;
    TuiAudioStream(TuiAudioStream&&) = delete;
    TuiAudioStream& operator=(TuiAudioStream&&) = delete;

    /// @brief Initialize and start the background audio streaming thread.
    /// @return True if audio device opened successfully, false if in fallback/dummy mode.
    bool start() noexcept;

    /// @brief Stop background streaming thread and close audio device.
    void stop() noexcept;

    /// @brief Query whether the streaming thread is actively running.
    /// @return True if streaming active.
    [[nodiscard]] bool isRunning() const noexcept;

    /// @brief Query whether a hardware audio device was successfully acquired.
    /// @return True if hardware playback active, false if dummy.
    [[nodiscard]] bool isAvailable() const noexcept;

private:
    ArcadeAudio& m_audio;
    std::atomic<bool> m_running {false};
    bool m_available {false};
    std::thread m_thread {};

#ifdef QIX_HAS_ALSA
    void* m_pcmHandle {nullptr}; // snd_pcm_t*
#endif

    void audioThreadFunc() noexcept;
};

} // namespace qix::tui
