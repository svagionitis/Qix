#include "ParticleSystem.h"
#include <algorithm>
#include <cmath>

namespace qix {

namespace {
    constexpr float Pi = 3.14159265358979323846f;
    constexpr float TwoPi = 2.0f * Pi;
} // namespace

std::uint32_t ParticleSystem::randomUint() noexcept
{
    m_rngState ^= m_rngState << 13;
    m_rngState ^= m_rngState >> 17;
    m_rngState ^= m_rngState << 5;
    return m_rngState;
}

float ParticleSystem::randomFloat(float minVal, float maxVal) noexcept
{
    const float norm = static_cast<float>(randomUint() & 0x00FFFFFFU) / static_cast<float>(0x00FFFFFFU);
    return minVal + norm * (maxVal - minVal);
}

void ParticleSystem::clear() noexcept
{
    m_activeCount = 0;
    m_fuseAccumulator = 0.0f;
}

const Particle* ParticleSystem::data() const noexcept
{
    return m_pool.data();
}

std::size_t ParticleSystem::size() const noexcept
{
    return m_activeCount;
}

bool ParticleSystem::empty() const noexcept
{
    return m_activeCount == 0;
}

bool ParticleSystem::addParticle(const Particle& particle) noexcept
{
    if (m_activeCount >= MaxParticles) {
        return false;
    }
    m_pool[m_activeCount++] = particle;
    return true;
}

void ParticleSystem::update(float dtSeconds) noexcept
{
    const float dt = std::clamp(dtSeconds, 0.0f, 1.0f);
    if (dt <= 0.0f) {
        return;
    }

    std::size_t i = 0;
    while (i < m_activeCount) {
        auto& p = m_pool[i];
        p.life -= p.decay * dt;

        if (p.life <= 0.0f) {
            // O(1) swap with back and pop
            if (m_activeCount > 1 && i != m_activeCount - 1) {
                m_pool[i] = m_pool[m_activeCount - 1];
            }
            --m_activeCount;
            // Do not increment i; re-evaluate the swapped particle
        } else {
            // Apply drag deceleration
            const float drag = (p.type == ParticleType::Spark) ? 2.5f : 1.8f;
            const float dragFactor = std::max(0.0f, 1.0f - drag * dt);
            p.vx *= dragFactor;
            p.vy *= dragFactor;

            p.x += p.vx * dt;
            p.y += p.vy * dt;
            p.rotation += p.rotationSpeed * dt;
            ++i;
        }
    }
}

void ParticleSystem::emitFuseSparkles(Point fusePos, const std::vector<Point>& trail, float dtSeconds) noexcept
{
    m_fuseAccumulator += dtSeconds;
    constexpr float EmitInterval = 0.025f; // ~40 spark bursts per second

    // Determine trail heading vector if multiple points exist
    float trailDirX = 0.0f;
    float trailDirY = 0.0f;
    if (trail.size() >= 2) {
        const auto& pEnd = trail.back();
        const auto& pPrev = trail[trail.size() - 2];
        const float dx = static_cast<float>(pEnd.x - pPrev.x);
        const float dy = static_cast<float>(pEnd.y - pPrev.y);
        const float len = std::hypot(dx, dy);
        if (len > 0.001f) {
            trailDirX = dx / len;
            trailDirY = dy / len;
        }
    }

    while (m_fuseAccumulator >= EmitInterval) {
        m_fuseAccumulator -= EmitInterval;

        const std::size_t sparksToSpawn = static_cast<std::size_t>(randomFloat(2.0f, 4.0f));
        for (std::size_t s = 0; s < sparksToSpawn; ++s) {
            if (m_activeCount >= MaxParticles) {
                break;
            }

            const float angle = randomFloat(0.0f, TwoPi);
            const float speed = randomFloat(6.0f, 22.0f);

            // Backward bias along trail direction with radial divergence
            const float vx = std::cos(angle) * speed - trailDirX * 4.0f;
            const float vy = std::sin(angle) * speed - trailDirY * 4.0f;

            const float maxLife = randomFloat(0.15f, 0.35f);
            const float startX = static_cast<float>(fusePos.x) + 0.5f + randomFloat(-0.25f, 0.25f);
            const float startY = static_cast<float>(fusePos.y) + 0.5f + randomFloat(-0.25f, 0.25f);

            // Fiery gradient: white-hot/bright yellow decaying to hot red/orange
            const bool isWhiteHot = randomFloat(0.0f, 1.0f) < 0.45f;
            const PaletteColor startColor = isWhiteHot
                ? PaletteColor {255, 255, 240, 255}
                : PaletteColor {255, static_cast<std::uint8_t>(randomFloat(180.0f, 230.0f)), 30, 255};

            const PaletteColor endColor {230, static_cast<std::uint8_t>(randomFloat(20.0f, 60.0f)), 0, 0};

            Particle p {};
            p.x = startX;
            p.y = startY;
            p.vx = vx;
            p.vy = vy;
            p.size = randomFloat(0.25f, 0.55f);
            p.rotation = std::atan2(vy, vx);
            p.rotationSpeed = 0.0f;
            p.life = 1.0f;
            p.decay = 1.0f / maxLife;
            p.startColor = startColor;
            p.endColor = endColor;
            p.type = ParticleType::Spark;

            addParticle(p);
        }
    }
}

void ParticleSystem::emitCaptureFlash(const std::vector<Point>& perimeter, DrawMode mode) noexcept
{
    if (perimeter.empty()) {
        return;
    }

    const bool isSlow = (mode == DrawMode::Slow);
    // Slow Draw (2x score): incandescent gold/amber. Fast Draw (1x score): electric cyan/blue.
    const PaletteColor baseStartColor = isSlow ? PaletteColor {255, 230, 60, 255} : PaletteColor {0, 245, 255, 255};

    const PaletteColor baseEndColor = isSlow ? PaletteColor {255, 110, 0, 0} : PaletteColor {0, 70, 230, 0};

    // 1. Emit glowing burst shards along the completed Stix trail vertices
    const std::size_t step = std::max<std::size_t>(1, perimeter.size() / 32);
    for (std::size_t idx = 0; idx < perimeter.size(); idx += step) {
        if (m_activeCount >= MaxParticles) {
            break;
        }

        const auto& pt = perimeter[idx];
        const float px = static_cast<float>(pt.x) + 0.5f;
        const float py = static_cast<float>(pt.y) + 0.5f;

        const std::size_t shards = static_cast<std::size_t>(randomFloat(2.0f, 4.0f));
        for (std::size_t s = 0; s < shards; ++s) {
            const float angle = randomFloat(0.0f, TwoPi);
            const float speed = randomFloat(5.0f, 18.0f);
            const float maxLife = randomFloat(0.35f, 0.70f);

            Particle p {};
            p.x = px + randomFloat(-0.2f, 0.2f);
            p.y = py + randomFloat(-0.2f, 0.2f);
            p.vx = std::cos(angle) * speed;
            p.vy = std::sin(angle) * speed;
            p.size = randomFloat(0.4f, 0.85f);
            p.rotation = randomFloat(0.0f, TwoPi);
            p.rotationSpeed = randomFloat(-8.0f, 8.0f);
            p.life = 1.0f;
            p.decay = 1.0f / maxLife;
            p.startColor = baseStartColor;
            p.endColor = baseEndColor;
            p.type = ParticleType::GlowShard;

            addParticle(p);
        }
    }

    // 2. Concentric radial shockwave burst at the closure anchor point
    const auto& closurePt = perimeter.back();
    const float cx = static_cast<float>(closurePt.x) + 0.5f;
    const float cy = static_cast<float>(closurePt.y) + 0.5f;
    constexpr std::size_t ShockwaveRays = 24;

    for (std::size_t r = 0; r < ShockwaveRays; ++r) {
        if (m_activeCount >= MaxParticles) {
            break;
        }

        const float angle
            = (static_cast<float>(r) / static_cast<float>(ShockwaveRays)) * TwoPi + randomFloat(-0.1f, 0.1f);
        const float speed = randomFloat(12.0f, 26.0f);
        const float maxLife = randomFloat(0.45f, 0.85f);

        Particle p {};
        p.x = cx;
        p.y = cy;
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.size = randomFloat(0.5f, 1.1f);
        p.rotation = randomFloat(0.0f, TwoPi);
        p.rotationSpeed = randomFloat(-12.0f, 12.0f);
        p.life = 1.0f;
        p.decay = 1.0f / maxLife;
        p.startColor = PaletteColor {255, 255, 255, 255};
        p.endColor = baseEndColor;
        p.type = (r % 2 == 0) ? ParticleType::DebrisDiamond : ParticleType::Spark;

        addParticle(p);
    }
}

void ParticleSystem::emitMarkerExplosion(Point deathPos) noexcept
{
    const float cx = static_cast<float>(deathPos.x) + 0.5f;
    const float cy = static_cast<float>(deathPos.y) + 0.5f;

    // Vibrant arcade neon multi-color palette
    const std::array<PaletteColor, 6> neonColors {
        PaletteColor {255, 255, 255, 255}, // White core
        PaletteColor {255, 235, 30, 255}, // Electric yellow
        PaletteColor {0, 245, 255, 255}, // Cyan
        PaletteColor {255, 30, 140, 255}, // Magenta
        PaletteColor {255, 45, 30, 255}, // Crimson
        PaletteColor {40, 255, 110, 255} // Lime emerald
    };

    constexpr std::size_t TotalDebris = 44;
    for (std::size_t i = 0; i < TotalDebris; ++i) {
        if (m_activeCount >= MaxParticles) {
            break;
        }

        const float angle
            = (static_cast<float>(i) / static_cast<float>(TotalDebris)) * TwoPi + randomFloat(-0.15f, 0.15f);
        const float speed = randomFloat(8.0f, 32.0f);
        const float maxLife = randomFloat(0.6f, 1.25f);
        const auto& startCol = neonColors[i % neonColors.size()];
        const PaletteColor endCol {startCol.r, startCol.g, startCol.b, 0};

        // Distribute across geometric debris types
        ParticleType debrisType = ParticleType::DebrisDiamond;
        if (i % 3 == 1) {
            debrisType = ParticleType::DebrisSquare;
        } else if (i % 3 == 2) {
            debrisType = ParticleType::DebrisLine;
        }

        Particle p {};
        p.x = cx;
        p.y = cy;
        p.vx = std::cos(angle) * speed;
        p.vy = std::sin(angle) * speed;
        p.size = randomFloat(0.4f, 1.0f);
        p.rotation = randomFloat(0.0f, TwoPi);
        p.rotationSpeed = randomFloat(-16.0f, 16.0f);
        p.life = 1.0f;
        p.decay = 1.0f / maxLife;
        p.startColor = startCol;
        p.endColor = endCol;
        p.type = debrisType;

        addParticle(p);
    }
}

} // namespace qix
