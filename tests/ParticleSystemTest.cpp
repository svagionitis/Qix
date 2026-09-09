#include "ParticleSystem.h"
#include "QixGame.h"
#include <gtest/gtest.h>

TEST(ParticleSystemTest, PoolLifecycleAndZeroAllocation)
{
    qix::ParticleSystem ps {};
    EXPECT_TRUE(ps.empty());
    EXPECT_EQ(ps.size(), 0U);

    qix::Particle p1 {};
    p1.x = 10.0f;
    p1.y = 10.0f;
    p1.vx = 5.0f;
    p1.vy = 0.0f;
    p1.life = 1.0f;
    p1.decay = 2.0f; // Dies in 0.5 seconds
    p1.type = qix::ParticleType::Spark;
    EXPECT_TRUE(ps.addParticle(p1));
    EXPECT_EQ(ps.size(), 1U);
    EXPECT_FALSE(ps.empty());

    // Step 0.25s: life becomes 0.5f, position advances
    ps.update(0.25f);
    EXPECT_EQ(ps.size(), 1U);
    EXPECT_GT(ps.data()[0].x, 10.0f);
    EXPECT_FLOAT_EQ(ps.data()[0].life, 0.5f);

    // Step another 0.3s: particle should decay and be recycled
    ps.update(0.30f);
    EXPECT_EQ(ps.size(), 0U);
    EXPECT_TRUE(ps.empty());

    // Verify capacity limit clamping
    for (std::size_t i = 0; i < qix::ParticleSystem::MaxParticles; ++i) {
        EXPECT_TRUE(ps.addParticle(p1));
    }
    EXPECT_EQ(ps.size(), qix::ParticleSystem::MaxParticles);
    EXPECT_FALSE(ps.addParticle(p1)); // Over-capacity rejected

    ps.clear();
    EXPECT_TRUE(ps.empty());
}

TEST(ParticleSystemTest, FuseSparklesGeneration)
{
    qix::ParticleSystem ps {};
    const qix::Point fusePos {20, 30};
    const std::vector<qix::Point> trail {qix::Point {18, 30}, qix::Point {19, 30}, qix::Point {20, 30}};

    // Emit sparks across 100ms
    ps.emitFuseSparkles(fusePos, trail, 0.10f);
    EXPECT_GT(ps.size(), 0U);

    for (std::size_t i = 0; i < ps.size(); ++i) {
        const auto& p = ps.data()[i];
        EXPECT_EQ(p.type, qix::ParticleType::Spark);
        EXPECT_NEAR(p.x, 20.5f, 1.5f);
        EXPECT_NEAR(p.y, 30.5f, 1.5f);
        // Fiery palette check: high red channel
        EXPECT_GE(p.startColor.r, 200U);
    }
}

TEST(ParticleSystemTest, CapturePerimeterFlashColorsAndShockwave)
{
    qix::ParticleSystem psSlow {};
    qix::ParticleSystem psFast {};

    const std::vector<qix::Point> perimeter {
        qix::Point {10, 10}, qix::Point {10, 20}, qix::Point {20, 20}, qix::Point {20, 10}};

    psSlow.emitCaptureFlash(perimeter, qix::DrawMode::Slow);
    psFast.emitCaptureFlash(perimeter, qix::DrawMode::Fast);

    EXPECT_GT(psSlow.size(), 0U);
    EXPECT_GT(psFast.size(), 0U);

    // Slow draw uses amber/gold (high red and green)
    const auto& pSlow = psSlow.data()[0];
    EXPECT_GE(pSlow.startColor.r, 200U);
    EXPECT_GE(pSlow.startColor.g, 150U);

    // Fast draw uses electric cyan (high green and blue)
    const auto& pFast = psFast.data()[0];
    EXPECT_GE(pFast.startColor.b, 200U);
    EXPECT_GE(pFast.startColor.g, 200U);
}

TEST(ParticleSystemTest, MarkerExplosionDebrisScatter)
{
    qix::ParticleSystem ps {};
    const qix::Point deathPos {40, 30};

    ps.emitMarkerExplosion(deathPos);
    EXPECT_GT(ps.size(), 20U);

    bool hasDiamond = false;
    bool hasSquare = false;
    bool hasLine = false;
    bool hasSpin = false;

    for (std::size_t i = 0; i < ps.size(); ++i) {
        const auto& p = ps.data()[i];
        EXPECT_NEAR(p.x, 40.5f, 0.1f);
        EXPECT_NEAR(p.y, 30.5f, 0.1f);

        if (p.type == qix::ParticleType::DebrisDiamond) {
            hasDiamond = true;
        } else if (p.type == qix::ParticleType::DebrisSquare) {
            hasSquare = true;
        } else if (p.type == qix::ParticleType::DebrisLine) {
            hasLine = true;
        }

        if (std::abs(p.rotationSpeed) > 0.1f) {
            hasSpin = true;
        }
    }

    EXPECT_TRUE(hasDiamond);
    EXPECT_TRUE(hasSquare);
    EXPECT_TRUE(hasLine);
    EXPECT_TRUE(hasSpin);

    // Update kinematics and verify dispersion
    ps.update(0.1f);
    float maxDistSq = 0.0f;
    for (std::size_t i = 0; i < ps.size(); ++i) {
        const auto& p = ps.data()[i];
        const float dx = p.x - 40.5f;
        const float dy = p.y - 30.5f;
        const float distSq = dx * dx + dy * dy;
        if (distSq > maxDistSq) {
            maxDistSq = distSq;
        }
    }
    EXPECT_GT(maxDistSq, 0.5f);
}

TEST(ParticleSystemTest, QixGameEventsPropagation)
{
    qix::QixGame game {};
    EXPECT_TRUE(game.getView().events.empty());

    // Step with input to transition from Ready to Playing
    game.handleInput(qix::PlayerCommand {qix::Direction::Right, qix::DrawMode::Slow});
    game.step(50);
    EXPECT_EQ(game.getView().state, qix::GameState::Playing);
}
