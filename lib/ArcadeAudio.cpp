#include "ArcadeAudio.h"
#include <algorithm>
#include <cmath>

namespace qix {

namespace {
    constexpr double kTwoPi = 6.28318530717958647692;
} // namespace

ArcadeAudio::ArcadeAudio(bool startMuted) noexcept
    : m_muted {startMuted}
{
}

void ArcadeAudio::setMuted(bool muted) noexcept
{
    m_muted.store(muted, std::memory_order_relaxed);
}

bool ArcadeAudio::isMuted() const noexcept
{
    return m_muted.load(std::memory_order_relaxed);
}

void ArcadeAudio::toggleMute() noexcept
{
    m_muted.store(!m_muted.load(std::memory_order_relaxed), std::memory_order_relaxed);
}

void ArcadeAudio::setMasterVolume(float volume) noexcept
{
    m_masterVolume.store(std::clamp(volume, 0.0f, 1.0f), std::memory_order_relaxed);
}

float ArcadeAudio::getMasterVolume() const noexcept
{
    return m_masterVolume.load(std::memory_order_relaxed);
}

void ArcadeAudio::triggerLevelComplete() noexcept
{
    m_pendingParams.triggerFanfare = true;
    (void)m_paramQueue.tryPush(m_pendingParams);
    m_pendingParams.triggerFanfare = false;
}

void ArcadeAudio::triggerPlayerDeath() noexcept
{
    m_pendingParams.triggerDeath = true;
    (void)m_paramQueue.tryPush(m_pendingParams);
    m_pendingParams.triggerDeath = false;
}

void ArcadeAudio::update(const GameView& view, std::uint32_t /*deltaMs*/) noexcept
{
    // Automatic State Transition Detection for Jingles (Game Thread)
    if (m_prevGameState != GameState::LevelComplete && view.state == GameState::LevelComplete) {
        m_pendingParams.triggerFanfare = true;
    }
    if (view.stats.lives < m_prevLives
        || (m_prevGameState == GameState::Playing && view.state == GameState::GameOver)) {
        m_pendingParams.triggerDeath = true;
    }
    m_prevGameState = view.state;
    m_prevLives = view.stats.lives;
    m_prevLevel = view.stats.level;
    m_pendingParams.qixTrapped = view.stats.qixTrapped;

    if (view.isPaused) {
        m_pendingParams.qixHumActive = false;
        m_pendingParams.drawingActive = false;
        m_pendingParams.fuseActive = false;
        m_pendingParams.sparxActive = false;
        (void)m_paramQueue.tryPush(m_pendingParams);
        m_pendingParams.triggerFanfare = false;
        m_pendingParams.triggerDeath = false;
        return;
    }

    // 1. Qix Hum Parameters
    if (view.state == GameState::Playing && !view.qixRibbons.empty() && !view.qixRibbons[0].empty()) {
        m_pendingParams.qixHumActive = true;
        const auto& head = view.qixRibbons[0].front();
        const double dx = static_cast<double>(head.start.x - head.end.x);
        const double dy = static_cast<double>(head.start.y - head.end.y);
        m_pendingParams.qixSegmentLength = static_cast<float>(std::hypot(dx, dy));
    } else {
        m_pendingParams.qixHumActive = false;
    }

    // 2. Drawing Chirp Parameters
    if (view.state == GameState::Playing && view.drawMode != DrawMode::None) {
        m_pendingParams.drawingActive = true;
        m_pendingParams.drawMode = view.drawMode;
    } else {
        m_pendingParams.drawingActive = false;
        m_pendingParams.drawMode = DrawMode::None;
    }

    // 3. Fuse Sizzle Parameters
    if (view.state == GameState::Playing && view.fusePos.has_value()) {
        m_pendingParams.fuseActive = true;
        const double dx = static_cast<double>(view.markerPos.x - view.fusePos->x);
        const double dy = static_cast<double>(view.markerPos.y - view.fusePos->y);
        m_pendingParams.fuseDistance = static_cast<float>(std::hypot(dx, dy));
    } else {
        m_pendingParams.fuseActive = false;
    }

    // 4. Sparx Siren Parameters
    if (view.state == GameState::Playing) {
        m_pendingParams.timeUp = view.stats.timeUp;

        // Calculate distance to nearest Sparx
        float minDist = 100.0f;
        for (const auto& sparxPos : view.sparxPositions) {
            const double dx = static_cast<double>(view.markerPos.x - sparxPos.x);
            const double dy = static_cast<double>(view.markerPos.y - sparxPos.y);
            const float d = static_cast<float>(std::hypot(dx, dy));
            if (d < minDist) {
                minDist = d;
            }
        }

        constexpr float kWarningRadius = 18.0f;
        if (m_pendingParams.timeUp || minDist < kWarningRadius) {
            m_pendingParams.sparxActive = true;
            m_pendingParams.sparxProximity = std::clamp(minDist / kWarningRadius, 0.0f, 1.0f);
        } else {
            m_pendingParams.sparxActive = false;
        }
    } else {
        m_pendingParams.sparxActive = false;
        m_pendingParams.timeUp = false;
    }

    (void)m_paramQueue.tryPush(m_pendingParams);
    m_pendingParams.triggerFanfare = false;
    m_pendingParams.triggerDeath = false;
}

float ArcadeAudio::softClip(float x) noexcept
{
    if (x > 1.5f) {
        return 1.0f;
    }
    if (x < -1.5f) {
        return -1.0f;
    }
    if (std::abs(x) < 1.0f) {
        return (x - (x * x * x) / 3.0f) * 1.5f;
    }
    return (x > 0.0f) ? 1.0f : -1.0f;
}

void ArcadeAudio::generateSamples(std::int16_t* output, std::size_t sampleCount) noexcept
{
    if (output == nullptr || sampleCount == 0) {
        return;
    }

    // Drain all pending updates from game thread into active parameters (Audio Thread)
    SynthParams update {};
    while (m_paramQueue.tryPop(update)) {
        if (update.triggerFanfare) {
            m_voice.fanfarePlaying = true;
            m_voice.fanfareSampleIndex = 0;
        }
        if (update.triggerDeath) {
            m_voice.deathPlaying = true;
            m_voice.deathSampleIndex = 0;
        }
        m_activeParams = update;
    }

    const bool muted = m_muted.load(std::memory_order_relaxed);
    const float masterVolume = m_masterVolume.load(std::memory_order_relaxed);

    if (muted || masterVolume <= 0.0f) {
        std::fill_n(output, sampleCount, static_cast<std::int16_t>(0));
        return;
    }

    constexpr double dt = 1.0 / static_cast<double>(SampleRate);

    for (std::size_t s {0}; s < sampleCount; ++s) {
        float mix = 0.0f;

        // -------------------------------------------------------------
        // 1. The Qix Hum (Dual detuned pulse oscillators with PWM)
        // -------------------------------------------------------------
        if (m_activeParams.qixHumActive) {
            // Slower, longer line segment produces lower pitch, shorter produces higher pitch
            const double targetFreq
                = 105.0 - std::clamp(static_cast<double>(m_activeParams.qixSegmentLength), 5.0, 55.0);
            m_voice.humCurrentFreq += (targetFreq - m_voice.humCurrentFreq) * 0.002;

            m_voice.humLfoPhase += kTwoPi * 3.5 * dt;
            if (m_voice.humLfoPhase >= kTwoPi) {
                m_voice.humLfoPhase -= kTwoPi;
            }
            const double duty = 0.5 + 0.22 * std::sin(m_voice.humLfoPhase);

            m_voice.humPhase1 += kTwoPi * m_voice.humCurrentFreq * dt;
            if (m_voice.humPhase1 >= kTwoPi) {
                m_voice.humPhase1 -= kTwoPi;
            }

            m_voice.humPhase2 += kTwoPi * (m_voice.humCurrentFreq * 1.503) * dt;
            if (m_voice.humPhase2 >= kTwoPi) {
                m_voice.humPhase2 -= kTwoPi;
            }

            const float osc1 = (m_voice.humPhase1 < duty * kTwoPi) ? 0.25f : -0.25f;
            const float osc2 = (m_voice.humPhase2 < 0.5 * kTwoPi) ? 0.15f : -0.15f;
            mix += (osc1 + osc2);
        }

        // -------------------------------------------------------------
        // 2. Drawing Chirp (Distinct click tempos for Slow vs. Fast)
        // -------------------------------------------------------------
        if (m_activeParams.drawingActive) {
            const bool isFast = (m_activeParams.drawMode == DrawMode::Fast);
            // Fast draw: 18 clicks/sec, 950 Hz tone
            // Slow draw: 9 clicks/sec (authentic half-speed), 550 Hz tone
            const std::uint32_t interval = isFast ? (SampleRate / 18U) : (SampleRate / 9U);
            const double chirpTone = isFast ? 950.0 : 550.0;

            if (m_voice.chirpSampleTimer == 0 && m_voice.chirpEnvelope <= 0.001) {
                m_voice.chirpEnvelope = 1.0;
            }

            ++m_voice.chirpSampleTimer;
            if (m_voice.chirpSampleTimer >= interval) {
                m_voice.chirpSampleTimer = 0;
                m_voice.chirpEnvelope = 1.0;
            }

            m_voice.chirpPhase += kTwoPi * chirpTone * dt;
            if (m_voice.chirpPhase >= kTwoPi) {
                m_voice.chirpPhase -= kTwoPi;
            }

            m_voice.chirpEnvelope *= 0.9982; // Rapid exponential decay (~12ms click)
            if (m_voice.chirpEnvelope < 0.001) {
                m_voice.chirpEnvelope = 0.0;
            }
            const float chirpPulse = (m_voice.chirpPhase < 0.5 * kTwoPi) ? 0.35f : -0.35f;
            mix += chirpPulse * static_cast<float>(m_voice.chirpEnvelope);
        } else {
            m_voice.chirpSampleTimer = 0;
            m_voice.chirpEnvelope = 0.0;
        }

        // -------------------------------------------------------------
        // 3. Fuse Sizzle (High-pass filtered noise with pitch escalation)
        // -------------------------------------------------------------
        if (m_activeParams.fuseActive) {
            // Escalation factor: 0.0 (far away) to 1.0 (touching marker)
            const float normDist = 1.0f - std::clamp(m_activeParams.fuseDistance / 70.0f, 0.0f, 1.0f);
            const double cutoffFreq = 2200.0 + static_cast<double>(normDist) * 4800.0;

            // 16-bit Galois LFSR pseudo-random noise generator
            m_voice.noiseLfsr = (m_voice.noiseLfsr >> 1) ^ (-(m_voice.noiseLfsr & 1U) & 0xB400U);
            const float rawNoise = (static_cast<float>(m_voice.noiseLfsr) / 32768.0f) - 1.0f;

            // 1-pole high-pass filter
            const double alpha = std::clamp(kTwoPi * cutoffFreq * dt, 0.01, 0.99);
            m_voice.fuseFilterLp += alpha * (static_cast<double>(rawNoise) - m_voice.fuseFilterLp);
            const float hpNoise = rawNoise - static_cast<float>(m_voice.fuseFilterLp);

            // Crackle pulse modulation (buzzing sparks)
            m_voice.fusePulsePhase += kTwoPi * (100.0 + static_cast<double>(normDist) * 150.0) * dt;
            if (m_voice.fusePulsePhase >= kTwoPi) {
                m_voice.fusePulsePhase -= kTwoPi;
            }
            const float sparkGate = (m_voice.fusePulsePhase < 0.4 * kTwoPi) ? 1.0f : 0.25f;

            mix += hpNoise * sparkGate * (0.20f + normDist * 0.30f);
        }

        // -------------------------------------------------------------
        // 4. Sparx Siren (Piercing FM warble)
        // -------------------------------------------------------------
        if (m_activeParams.sparxActive) {
            const double lfoFreq = m_activeParams.timeUp ? 16.0 : 9.0;
            const double modDepth = m_activeParams.timeUp ? 280.0 : 160.0;
            const double centerFreq = m_activeParams.timeUp ? 880.0 : 780.0;

            m_voice.sparxLfoPhase += kTwoPi * lfoFreq * dt;
            if (m_voice.sparxLfoPhase >= kTwoPi) {
                m_voice.sparxLfoPhase -= kTwoPi;
            }

            const double instFreq = centerFreq + modDepth * std::sin(m_voice.sparxLfoPhase);
            m_voice.sparxPhase += kTwoPi * instFreq * dt;
            if (m_voice.sparxPhase >= kTwoPi) {
                m_voice.sparxPhase -= kTwoPi;
            }

            const float amp = m_activeParams.timeUp ? 0.35f : (1.0f - m_activeParams.sparxProximity) * 0.30f;
            const float sirenWave = (m_voice.sparxPhase < 0.5 * kTwoPi) ? 1.0f : -1.0f;
            mix += sirenWave * amp;
        }

        // -------------------------------------------------------------
        // 5. Fanfare Jingle (Ascending 4-note chord arpeggio)
        // -------------------------------------------------------------
        if (m_voice.fanfarePlaying) {
            constexpr std::uint32_t kTotalFanfareSamples = static_cast<std::uint32_t>(SampleRate * 1.2);
            constexpr std::uint32_t kNoteDuration = SampleRate / 7U; // ~142ms per note
            const double trapPitchScale = m_activeParams.qixTrapped ? 1.5 : 1.0;

            if (m_voice.fanfareSampleIndex < kTotalFanfareSamples) {
                const auto noteIdx = m_voice.fanfareSampleIndex / kNoteDuration;
                float fanfareSample = 0.0f;

                if (noteIdx == 0) {
                    m_voice.fanfarePhase1 += kTwoPi * 659.25 * trapPitchScale * dt; // E5
                    fanfareSample = (m_voice.fanfarePhase1 < 0.5 * kTwoPi) ? 0.35f : -0.35f;
                } else if (noteIdx == 1) {
                    m_voice.fanfarePhase2 += kTwoPi * 830.61 * trapPitchScale * dt; // G#5
                    fanfareSample = (m_voice.fanfarePhase2 < 0.5 * kTwoPi) ? 0.35f : -0.35f;
                } else if (noteIdx == 2) {
                    m_voice.fanfarePhase3 += kTwoPi * 987.77 * trapPitchScale * dt; // B5
                    fanfareSample = (m_voice.fanfarePhase3 < 0.5 * kTwoPi) ? 0.35f : -0.35f;
                } else if (noteIdx == 3) {
                    m_voice.fanfarePhase4 += kTwoPi * 1318.51 * trapPitchScale * dt; // E6
                    fanfareSample = (m_voice.fanfarePhase4 < 0.5 * kTwoPi) ? 0.35f : -0.35f;
                } else {
                    // Sustained chord of all 4 notes decaying
                    const double remainingFrac = 1.0
                        - static_cast<double>(m_voice.fanfareSampleIndex - 4U * kNoteDuration)
                            / static_cast<double>(kTotalFanfareSamples - 4U * kNoteDuration);
                    const float chordDecay = static_cast<float>(std::clamp(remainingFrac, 0.0, 1.0));

                    m_voice.fanfarePhase1 += kTwoPi * 659.25 * trapPitchScale * dt;
                    m_voice.fanfarePhase2 += kTwoPi * 830.61 * trapPitchScale * dt;
                    m_voice.fanfarePhase3 += kTwoPi * 987.77 * trapPitchScale * dt;
                    m_voice.fanfarePhase4 += kTwoPi * 1318.51 * trapPitchScale * dt;

                    const float c1 = (m_voice.fanfarePhase1 < 0.5 * kTwoPi) ? 0.12f : -0.12f;
                    const float c2 = (m_voice.fanfarePhase2 < 0.5 * kTwoPi) ? 0.12f : -0.12f;
                    const float c3 = (m_voice.fanfarePhase3 < 0.5 * kTwoPi) ? 0.12f : -0.12f;
                    const float c4 = (m_voice.fanfarePhase4 < 0.5 * kTwoPi) ? 0.12f : -0.12f;
                    fanfareSample = (c1 + c2 + c3 + c4) * chordDecay;
                }

                mix += fanfareSample;
                ++m_voice.fanfareSampleIndex;
            } else {
                m_voice.fanfarePlaying = false;
            }
        }

        // -------------------------------------------------------------
        // 6. Death Jingle (Descending chromatic retro slide)
        // -------------------------------------------------------------
        if (m_voice.deathPlaying) {
            constexpr std::uint32_t kTotalDeathSamples = static_cast<std::uint32_t>(SampleRate * 0.7);

            if (m_voice.deathSampleIndex < kTotalDeathSamples) {
                const double progress
                    = static_cast<double>(m_voice.deathSampleIndex) / static_cast<double>(kTotalDeathSamples);
                // Pitch falls from 460 Hz down to 60 Hz
                const double deathFreq = 460.0 * std::exp(-progress * 2.2);
                const float decay = static_cast<float>(1.0 - progress);

                m_voice.deathPhase += kTwoPi * deathFreq * dt;
                if (m_voice.deathPhase >= kTwoPi) {
                    m_voice.deathPhase -= kTwoPi;
                }

                // Bit-crushed square wave with noise crackle
                m_voice.noiseLfsr = (m_voice.noiseLfsr >> 1) ^ (-(m_voice.noiseLfsr & 1U) & 0xB400U);
                const float noiseCrack = ((m_voice.noiseLfsr & 0x0FU) == 0U) ? 0.2f : 0.0f;
                const float squareTone = (m_voice.deathPhase < 0.5 * kTwoPi) ? 0.40f : -0.40f;

                mix += (squareTone + noiseCrack) * decay;
                ++m_voice.deathSampleIndex;
            } else {
                m_voice.deathPlaying = false;
            }
        }

        // -------------------------------------------------------------
        // Master Bus Saturation & Volume Scaling
        // -------------------------------------------------------------
        const float saturated = softClip(mix) * masterVolume;
        const auto pcm = static_cast<std::int16_t>(std::clamp(saturated * 32760.0f, -32760.0f, 32760.0f));
        output[s] = pcm;
    }
}

} // namespace qix
