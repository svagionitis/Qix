#include "CollisionDetector.h"
#include <gtest/gtest.h>

TEST(CollisionTest, SparxHitsMarker)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {5, 0}, 3};
    std::vector<qix::Qix> qixList {};
    std::vector<qix::Sparx> sparxList {qix::Sparx {qix::Point {5, 0}}};
    qix::Fuse fuse {};

    const auto event = qix::CollisionDetector::check(marker, qixList, sparxList, fuse);
    EXPECT_EQ(event, qix::CollisionEvent::MarkerHitBySparx);
}

TEST(CollisionTest, QixHitsActiveStix)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {10, 0}, 3};

    // Draw stix down into field
    marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Slow});
    marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Slow});
    EXPECT_TRUE(marker.isDrawing());

    // Qix line overlapping with trail point (10, 1)
    qix::LineSegment seg {qix::Point {8, 1}, qix::Point {12, 1}};
    std::vector<qix::Qix> qixList {qix::Qix {seg}};
    std::vector<qix::Sparx> sparxList {};
    qix::Fuse fuse {};

    const auto event = qix::CollisionDetector::check(marker, qixList, sparxList, fuse);
    EXPECT_EQ(event, qix::CollisionEvent::StixHitByQix);
}

TEST(CollisionTest, FuseHitsMarker)
{
    qix::Fuse fuse {2}; // Ignites after 2 idle ticks
    std::vector<qix::Point> trail {qix::Point {5, 0}, qix::Point {5, 1}};

    // Marker stationary while drawing for 2 ticks
    fuse.update(true, false, trail);
    EXPECT_FALSE(fuse.isBurning());

    fuse.update(true, false, trail);
    EXPECT_TRUE(fuse.isBurning());

    // Fuse advances to trail[0] (5, 0)
    EXPECT_EQ(fuse.getPosition(), (qix::Point {5, 0}));

    // Next tick fuse advances to trail[1] (5, 1)
    fuse.update(true, false, trail);
    EXPECT_EQ(fuse.getPosition(), (qix::Point {5, 1}));
    EXPECT_TRUE(fuse.checkCollision(qix::Point {5, 1}));
}

TEST(CollisionTest, SparxClassicVsModernTraversal)
{
    qix::Playfield field {20, 20};
    // (5, 1) is claimed territory
    field.setCell(5, 1, qix::CellState::ClaimedSlow);

    // Classic Sparx moving along top border at (5, 0)
    qix::Sparx classicSparx {qix::Point {5, 0}, true, qix::GameMode::Classic};
    classicSparx.update(field);
    // Should NOT enter (5, 1)
    EXPECT_NE(classicSparx.getPosition(), (qix::Point {5, 1}));

    // Modern Sparx moving along top border at (5, 0)
    qix::Sparx modernSparx {qix::Point {5, 0}, true, qix::GameMode::Modern};
    modernSparx.update(field);
    // Clockwise search from Direction::Right will check Down (5, 1) first, which is accepted in Modern
    EXPECT_EQ(modernSparx.getPosition(), (qix::Point {5, 1}));
}

TEST(CollisionTest, SuperSparxTraversesActiveStix)
{
    qix::Playfield field {20, 20};
    // Active Stix trail extending down from (5, 0)
    field.setCell(5, 1, qix::CellState::ActiveStix);
    field.setCell(5, 2, qix::CellState::ActiveStix);

    qix::Sparx superSparx {qix::Point {5, 0}, true, qix::GameMode::Classic, true};
    EXPECT_TRUE(superSparx.isSuper());

    // Step 1: Detects ActiveStix at (5, 1) and enters trail
    superSparx.update(field);
    EXPECT_EQ(superSparx.getPosition(), (qix::Point {5, 1}));

    // Step 2: Advances down trail to (5, 2)
    superSparx.update(field);
    EXPECT_EQ(superSparx.getPosition(), (qix::Point {5, 2}));
}

TEST(CollisionTest, RegularSparxIgnoresActiveStix)
{
    qix::Playfield field {20, 20};
    // Active Stix trail at (5, 1)
    field.setCell(5, 1, qix::CellState::ActiveStix);

    // Regular Sparx at (5, 0)
    qix::Sparx regularSparx {qix::Point {5, 0}, true, qix::GameMode::Classic, false};
    EXPECT_FALSE(regularSparx.isSuper());

    // Update must NOT enter (5, 1)
    regularSparx.update(field);
    EXPECT_NE(regularSparx.getPosition(), (qix::Point {5, 1}));
    EXPECT_EQ(regularSparx.getPosition(), (qix::Point {6, 0}));
}

TEST(CollisionTest, SuperSparxHitsMarkerOnStix)
{
    qix::Playfield field {20, 20};
    qix::Marker marker {qix::Point {5, 0}, 3};
    // Draw stix down into field to (5, 1)
    marker.move(field, qix::PlayerCommand {qix::Direction::Down, qix::DrawMode::Slow});
    EXPECT_TRUE(marker.isDrawing());
    EXPECT_EQ(marker.getPosition(), (qix::Point {5, 1}));

    std::vector<qix::Qix> qixList {};
    std::vector<qix::Sparx> sparxList {qix::Sparx {marker.getPosition(), true, qix::GameMode::Classic, true}};
    qix::Fuse fuse {};

    const auto event = qix::CollisionDetector::check(marker, qixList, sparxList, fuse);
    EXPECT_EQ(event, qix::CollisionEvent::MarkerHitBySparx);
}
