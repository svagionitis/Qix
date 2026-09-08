#pragma once
#include "IQixGame.h"
#include <cstdint>
#include <mutex>

namespace qix {

/// @class ArcadeAudio
/// @brief Procedural chiptune sound synthesis engine for authentic 1981 arcade audio.
/// @details Synthesizes real-time 44.1 kHz 16-bit mono PCM via mathematical waveform oscillators
/// (The Qix Hum, Drawing Chirp, Fuse Sizzle, Sparx Siren, Level Fanfare, and Death Jingle)
/// with zero external asset files required. Sound is muted by default.
class ArcadeAudio {
public:
    /// @brief Audio output sample rate in Hertz (CD quality standard).
    static constexpr std::uint32_t SampleRate {44100U};

    /// @brief Construct arcade audio synthesizer (muted by default).
    /// @param[in] startMuted Whether sound begins muted (default: true).
    explicit ArcadeAudio(bool startMuted = true) noexcept;

    ~ArcadeAudio() = default;

    // Non-copyable
    ArcadeAudio(const ArcadeAudio&) = delete;
    ArcadeAudio& operator=(const ArcadeAudio&) = delete;

    // Movable
    ArcadeAudio(ArcadeAudio&&) noexcept = default;
    ArcadeAudio& operator=(ArcadeAudio&&) noexcept = default;

    /// @brief Update internal acoustic parameters from immutable game snapshot.
    /// @details Called once per simulation step on the main game thread.
    /// @param[in] view Current game view containing Qix ribbons, marker, fuse, and sparx.
    /// @param[in] deltaMs Milliseconds elapsed since last step.
    void update(const GameView& view, std::uint32_t deltaMs) noexcept;

    /// @brief Trigger celebratory ascending arpeggio fanfare upon level completion.
    void triggerLevelComplete() noexcept;

    /// @brief Trigger retro descending chromatic frequency slide upon player death.
    void triggerPlayerDeath() noexcept;

    /// @brief Generate contiguous buffer of 16-bit signed mono PCM samples.
    /// @details Thread-safe method intended for real-time audio device callbacks or stream updates.
    /// @param[out] output Pointer to destination 16-bit integer buffer.
    /// @param[in] sampleCount Number of samples to synthesize.
    void generateSamples(std::int16_t* output, std::size_t sampleCount) noexcept;

    /// @brief Enable or disable audio output muting.
    /// @param[in] muted True to silence output, false to unmute.
    void setMuted(bool muted) noexcept;

    /// @brief Query whether audio output is currently muted.
    /// @return True if muted.
    [[nodiscard]] bool isMuted() const noexcept;

    /// @brief Toggle mute state on/off.
    void toggleMute() noexcept;

    /// @brief Set master volume multiplier [0.0f, 1.0f].
    /// @param[in] volume Clamped volume level.
    void setMasterVolume(float volume) noexcept;

    /// @brief Query current master volume level.
    /// @return Volume level in [0.0f, 1.0f].
    [[nodiscard]] float getMasterVolume() const noexcept;

private:
    mutable std::mutex m_mutex {};

    bool m_muted {true};
    float m_masterVolume {0.7f};

    // Synthesizer State (protected by m_mutex)
    struct SynthParams {
        bool qixHumActive {false};
        float qixSegmentLength {25.0f};

        bool drawingActive {false};
        DrawMode drawMode {DrawMode::None};

        bool fuseActive {false};
        float fuseDistance {100.0f};

        bool sparxActive {false};
        bool timeUp {false};
        float sparxProximity {1.0f}; // 0.0 (touching) to 1.0 (far away)

        bool triggerFanfare {false};
        bool triggerDeath {false};
    } m_params {};

    // Oscillator Phases & Internal Voice States (audio thread only)
    struct VoiceState {
        // Qix Hum (Dual detuned pulse oscillators with PWM)
        double humPhase1 {0.0};
        double humPhase2 {0.0};
        double humLfoPhase {0.0};
        double humCurrentFreq {80.0};

        // Drawing Chirp
        std::uint32_t chirpSampleTimer {0};
        double chirpPhase {0.0};
        double chirpEnvelope {0.0};

        // Fuse Sizzle
        std::uint32_t noiseLfsr {0xACE1U};
        double fuseFilterLp {0.0};
        double fusePulsePhase {0.0};

        // Sparx Siren
        double sparxPhase {0.0};
        double sparxLfoPhase {0.0};

        // Fanfare Jingle (Level Complete)
        bool fanfarePlaying {false};
        std::uint32_t fanfareSampleIndex {0};
        double fanfarePhase1 {0.0};
        double fanfarePhase2 {0.0};
        double fanfarePhase3 {0.0};
        double fanfarePhase4 {0.0};

        // Death Jingle
        bool deathPlaying {false};
        std::uint32_t deathSampleIndex {0};
        double deathPhase {0.0};

        // Previous Game View tracking for automatic edge detection
        GameState prevGameState {GameState::Ready};
        std::uint16_t prevLives {3};
        std::uint8_t prevLevel {1};
    } m_voice {};

    static float softClip(float x) noexcept;
};

} // namespace qix
