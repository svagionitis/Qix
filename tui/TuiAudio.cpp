#include "TuiAudio.h"
#include <array>
#include <chrono>

#ifdef QIX_HAS_ALSA
#include <alsa/asoundlib.h>
#endif

namespace qix::tui {

TuiAudioStream::TuiAudioStream(ArcadeAudio& audio) noexcept
    : m_audio {audio}
{
}

TuiAudioStream::~TuiAudioStream() noexcept
{
    stop();
}

bool TuiAudioStream::start() noexcept
{
    if (m_running.load(std::memory_order_acquire)) {
        return m_available;
    }

#ifdef QIX_HAS_ALSA
    snd_pcm_t* pcm {nullptr};
    const int openRes = snd_pcm_open(&pcm, "default", SND_PCM_STREAM_PLAYBACK, 0);
    if (openRes < 0 || pcm == nullptr) {
        m_available = false;
        m_running.store(false, std::memory_order_release);
        return false;
    }

    const int paramRes = snd_pcm_set_params(pcm, SND_PCM_FORMAT_S16_LE, SND_PCM_ACCESS_RW_INTERLEAVED, 1,
        ArcadeAudio::SampleRate, 1, 50000); // 50ms latency target
    if (paramRes < 0) {
        snd_pcm_close(pcm);
        m_available = false;
        m_running.store(false, std::memory_order_release);
        return false;
    }

    m_pcmHandle = pcm;
    m_available = true;
    m_running.store(true, std::memory_order_release);
    m_thread = std::thread(&TuiAudioStream::audioThreadFunc, this);
    return true;
#else
    m_available = false;
    m_running.store(false, std::memory_order_release);
    return false;
#endif
}

void TuiAudioStream::stop() noexcept
{
    if (m_running.exchange(false, std::memory_order_acq_rel)) {
#ifdef QIX_HAS_ALSA
        if (m_pcmHandle != nullptr) {
            auto* pcm = static_cast<snd_pcm_t*>(m_pcmHandle);
            snd_pcm_drop(pcm);
        }
#endif
        if (m_thread.joinable()) {
            m_thread.join();
        }
#ifdef QIX_HAS_ALSA
        if (m_pcmHandle != nullptr) {
            auto* pcm = static_cast<snd_pcm_t*>(m_pcmHandle);
            snd_pcm_close(pcm);
            m_pcmHandle = nullptr;
        }
#endif
        m_available = false;
    }
}

bool TuiAudioStream::isRunning() const noexcept
{
    return m_running.load(std::memory_order_relaxed);
}

bool TuiAudioStream::isAvailable() const noexcept
{
    return m_available;
}

void TuiAudioStream::audioThreadFunc() noexcept
{
#ifdef QIX_HAS_ALSA
    auto* pcm = static_cast<snd_pcm_t*>(m_pcmHandle);
    if (pcm == nullptr) {
        return;
    }

    constexpr std::size_t BufferSamples {1024U};
    std::array<std::int16_t, BufferSamples> buffer {};

    while (m_running.load(std::memory_order_relaxed)) {
        m_audio.generateSamples(buffer.data(), BufferSamples);
        snd_pcm_sframes_t frames = snd_pcm_writei(pcm, buffer.data(), BufferSamples);
        if (frames < 0) {
            frames = snd_pcm_recover(pcm, static_cast<int>(frames), 0);
            if (frames < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
    }
#endif
}

} // namespace qix::tui
