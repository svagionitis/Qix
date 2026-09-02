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
