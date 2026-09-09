#pragma once
#include "ColorPalette.h"
#include "Types.h"
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace qix {

/// @brief Geometric visual type of an individual simulated particle.
enum class ParticleType : std::uint8_t {
    Spark = 0,     ///< High-speed fiery spark / streak.
    GlowShard,     ///< Glowing diamond / boundary flash shard.
    DebrisDiamond, ///< Rotating multi-colored diamond fragment.
    DebrisSquare,  ///< Rotating square debris fragment.
    DebrisLine     ///< Tumbling geometric line shard.
};

/// @brief Individual particle instance with kinematics, orientation, and color decay.
struct Particle {
    float x {0.0f};             ///< Horizontal position in grid space.
    float y {0.0f};             ///< Vertical position in grid space.
    float vx {0.0f};            ///< Horizontal velocity in grid units per second.
    float vy {0.0f};            ///< Vertical velocity in grid units per second.
    float size {0.0f};          ///< Characteristic radius/scale in grid units.
    float rotation {0.0f};      ///< Angular rotation in radians.
    float rotationSpeed {0.0f}; ///< Angular velocity in radians per second.
    float life {1.0f};          ///< Normalized life remaining [1.0 -> 0.0].
    float decay {1.0f};         ///< Decay rate per second (1.0 / maxLife).
    PaletteColor startColor {}; ///< Color at birth.
    PaletteColor endColor {};   ///< Target color upon full decay.
    ParticleType type {ParticleType::Spark};

    /// @brief Interpolate current color based on remaining life.
    /// @return RGBA PaletteColor with blended RGB channels and alpha fade.
    [[nodiscard]] constexpr PaletteColor currentColor() const noexcept
    {
        const float t = life > 0.0f ? (life < 1.0f ? life : 1.0f) : 0.0f;
        const float invT = 1.0f - t;
        return PaletteColor {
            static_cast<std::uint8_t>(static_cast<float>(startColor.r) * t + static_cast<float>(endColor.r) * invT),
            static_cast<std::uint8_t>(static_cast<float>(startColor.g) * t + static_cast<float>(endColor.g) * invT),
            static_cast<std::uint8_t>(static_cast<float>(startColor.b) * t + static_cast<float>(endColor.b) * invT),
            static_cast<std::uint8_t>(static_cast<float>(startColor.a) * t + static_cast<float>(endColor.a) * invT)
        };
    }
};

/// @class ParticleSystem
/// @brief High-performance, zero-allocation particle simulation engine.
/// @details Maintains a fixed-capacity pool of 1024 particles, simulating physics
/// in playfield grid coordinates with O(1) swap-and-pop recycling and zero heap allocation.
class ParticleSystem {
public:
    /// @brief Maximum concurrent particles supported in memory pool.
    static constexpr std::size_t MaxParticles {1024};

    ParticleSystem() noexcept = default;
    ~ParticleSystem() = default;

    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;
    ParticleSystem(ParticleSystem&&) noexcept = default;
    ParticleSystem& operator=(ParticleSystem&&) noexcept = default;

    /// @brief Advance particle kinematics, rotational dynamics, and color decay.
    /// @param[in] dtSeconds Time delta in seconds elapsed since previous update.
    void update(float dtSeconds) noexcept;

    /// @brief Emit fiery sparks radiating from the active burning fuse head.
    /// @param[in] fusePos Current discrete grid coordinates of burning fuse.
    /// @param[in] trail Preceding Stix trail coordinates for directional drift.
    /// @param[in] dtSeconds Frame duration in seconds to pace emission density.
    void emitFuseSparkles(Point fusePos, const std::vector<Point>& trail, float dtSeconds) noexcept;

    /// @brief Trigger celebratory perimeter flash wave along newly enclosed territory boundary.
    /// @param[in] perimeter Trail points that closed the loop.
    /// @param[in] mode Drawing speed used (Slow produces amber/gold, Fast produces electric cyan).
    void emitCaptureFlash(const std::vector<Point>& perimeter, DrawMode mode) noexcept;

    /// @brief Trigger multi-colored 360-degree geometric debris scatter upon marker collision death.
    /// @param[in] deathPos Grid coordinate where collision occurred.
    void emitMarkerExplosion(Point deathPos) noexcept;

    /// @brief Retrieve contiguous array slice of currently active particles.
    /// @return Pointer to first active particle in pool.
    [[nodiscard]] const Particle* data() const noexcept;

    /// @brief Retrieve number of currently active particles.
    /// @return Active particle count (0 to MaxParticles).
    [[nodiscard]] std::size_t size() const noexcept;

    /// @brief Check whether any particles are currently active.
    /// @return True if size() == 0.
    [[nodiscard]] bool empty() const noexcept;

    /// @brief Reset and remove all active particles immediately.
    void clear() noexcept;

    /// @brief Add a single raw particle directly to the pool (if capacity permits).
    /// @param[in] particle Fully configured particle instance.
    /// @return True if successfully added, false if pool is full.
    bool addParticle(const Particle& particle) noexcept;

private:
    std::array<Particle, MaxParticles> m_pool {};
    std::size_t m_activeCount {0};
    float m_fuseAccumulator {0.0f};

    // Fast deterministic pseudo-random generator with zero heap overhead
    std::uint32_t m_rngState {0x12345678U};
    [[nodiscard]] float randomFloat(float minVal, float maxVal) noexcept;
    [[nodiscard]] std::uint32_t randomUint() noexcept;
};

} // namespace qix
