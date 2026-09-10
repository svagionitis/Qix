#include "MotionInterpolator.h"
#include <gtest/gtest.h>

using namespace qix;

TEST(MotionInterpolatorTest, CalculateAlphaBoundsAndClamping)
{
    // Normal in-range calculation
    EXPECT_FLOAT_EQ(MotionInterpolator::calculateAlpha(0.0, 0.05), 0.0f);
    EXPECT_FLOAT_EQ(MotionInterpolator::calculateAlpha(0.025, 0.05), 0.5f);
    EXPECT_FLOAT_EQ(MotionInterpolator::calculateAlpha(0.05, 0.05), 1.0f);

    // Clamping on overshoot
    EXPECT_FLOAT_EQ(MotionInterpolator::calculateAlpha(0.08, 0.05), 1.0f);

    // Clamping on negative time
    EXPECT_FLOAT_EQ(MotionInterpolator::calculateAlpha(-0.01, 0.05), 0.0f);

    // Edge case: zero or negative tick interval defaults to 1.0f
    EXPECT_FLOAT_EQ(MotionInterpolator::calculateAlpha(0.02, 0.0), 1.0f);
    EXPECT_FLOAT_EQ(MotionInterpolator::calculateAlpha(0.02, -0.05), 1.0f);
}

TEST(MotionInterpolatorTest, MarkerInterpolationWithoutHistory)
{
    MotionInterpolator interp {};
    EXPECT_FALSE(interp.hasPreviousState());

    const Point curr {15, 25};
    const auto [x, y] = interp.interpolateMarker(curr, 0.5f);
    EXPECT_FLOAT_EQ(x, 15.0f);
    EXPECT_FLOAT_EQ(y, 25.0f);
}

TEST(MotionInterpolatorTest, MarkerLinearInterpolation)
{
    MotionInterpolator interp {};

    GameView view {};
    view.markerPos = Point {10, 20};
    interp.onTick(view);

    EXPECT_TRUE(interp.hasPreviousState());
    EXPECT_EQ(interp.getPreviousMarker(), (Point {10, 20}));

    // Marker moved one step right
    const Point nextPos {11, 20};

    // At alpha = 0.0 (start of tick), position is previous
    {
        const auto [x, y] = interp.interpolateMarker(nextPos, 0.0f);
        EXPECT_FLOAT_EQ(x, 10.0f);
        EXPECT_FLOAT_EQ(y, 20.0f);
    }

    // At alpha = 0.25
    {
        const auto [x, y] = interp.interpolateMarker(nextPos, 0.25f);
        EXPECT_FLOAT_EQ(x, 10.25f);
        EXPECT_FLOAT_EQ(y, 20.0f);
    }

    // At alpha = 0.5 (midpoint)
    {
        const auto [x, y] = interp.interpolateMarker(nextPos, 0.5f);
        EXPECT_FLOAT_EQ(x, 10.5f);
        EXPECT_FLOAT_EQ(y, 20.0f);
    }

    // At alpha = 1.0 (end of tick)
    {
        const auto [x, y] = interp.interpolateMarker(nextPos, 1.0f);
        EXPECT_FLOAT_EQ(x, 11.0f);
        EXPECT_FLOAT_EQ(y, 20.0f);
    }
}

TEST(MotionInterpolatorTest, MarkerTeleportOrRespawnRejection)
{
    MotionInterpolator interp {};

    GameView view {};
    view.markerPos = Point {10, 20};
    interp.onTick(view);

    // Marker respawns at bottom edge (40, 59) -> jump > 1 cell
    const Point respawnPos {40, 59};

    // Even at alpha = 0.5f, it should NOT interpolate across the screen
    const auto [x, y] = interp.interpolateMarker(respawnPos, 0.5f);
    EXPECT_FLOAT_EQ(x, 40.0f);
    EXPECT_FLOAT_EQ(y, 59.0f);
}

TEST(MotionInterpolatorTest, SparxLinearInterpolation)
{
    MotionInterpolator interp {};

    GameView view {};
    view.sparxList.push_back(SparxInfo {Point {20, 0}, false});
    view.sparxList.push_back(SparxInfo {Point {60, 0}, true});
    interp.onTick(view);

    ASSERT_EQ(interp.getPreviousSparx().size(), 2U);
    EXPECT_EQ(interp.getPreviousSparx()[0], (Point {20, 0}));
    EXPECT_EQ(interp.getPreviousSparx()[1], (Point {60, 0}));

    // Sparx 0 moved clockwise to {21, 0}
    // Sparx 1 (super) moved counter-clockwise by 2 cells to {58, 0}
    const Point nextSparx0 {21, 0};
    const Point nextSparx1 {58, 0};

    // Alpha = 0.0f
    {
        const auto [x0, y0] = interp.interpolateSparx(0, nextSparx0, 0.0f);
        EXPECT_FLOAT_EQ(x0, 20.0f);
        EXPECT_FLOAT_EQ(y0, 0.0f);

        const auto [x1, y1] = interp.interpolateSparx(1, nextSparx1, 0.0f);
        EXPECT_FLOAT_EQ(x1, 60.0f);
        EXPECT_FLOAT_EQ(y1, 0.0f);
    }

    // Alpha = 0.5f
    {
        const auto [x0, y0] = interp.interpolateSparx(0, nextSparx0, 0.5f);
        EXPECT_FLOAT_EQ(x0, 20.5f);
        EXPECT_FLOAT_EQ(y0, 0.0f);

        const auto [x1, y1] = interp.interpolateSparx(1, nextSparx1, 0.5f);
        EXPECT_FLOAT_EQ(x1, 59.0f);
        EXPECT_FLOAT_EQ(y1, 0.0f);
    }

    // Alpha = 1.0f
    {
        const auto [x0, y0] = interp.interpolateSparx(0, nextSparx0, 1.0f);
        EXPECT_FLOAT_EQ(x0, 21.0f);
        EXPECT_FLOAT_EQ(y0, 0.0f);

        const auto [x1, y1] = interp.interpolateSparx(1, nextSparx1, 1.0f);
        EXPECT_FLOAT_EQ(x1, 58.0f);
        EXPECT_FLOAT_EQ(y1, 0.0f);
    }
}

TEST(MotionInterpolatorTest, SparxWrapOrDiscontinuityRejection)
{
    MotionInterpolator interp {};

    GameView view {};
    view.sparxList.push_back(SparxInfo {Point {0, 0}, false});
    interp.onTick(view);

    // Sparx wrapped or respawned across board to {70, 50} (> 2 units)
    const Point wrappedPos {70, 50};
    const auto [x, y] = interp.interpolateSparx(0, wrappedPos, 0.5f);
    EXPECT_FLOAT_EQ(x, 70.0f);
    EXPECT_FLOAT_EQ(y, 50.0f);

    // Out-of-bounds index returns current position
    const auto [xOut, yOut] = interp.interpolateSparx(99, Point {5, 5}, 0.5f);
    EXPECT_FLOAT_EQ(xOut, 5.0f);
    EXPECT_FLOAT_EQ(yOut, 5.0f);
}

TEST(MotionInterpolatorTest, ResetClearsHistory)
{
    MotionInterpolator interp {};

    GameView view {};
    view.markerPos = Point {10, 10};
    view.sparxPositions.push_back(Point {20, 20});
    interp.onTick(view);

    EXPECT_TRUE(interp.hasPreviousState());

    interp.reset();
    EXPECT_FALSE(interp.hasPreviousState());
    EXPECT_TRUE(interp.getPreviousSparx().empty());

    const auto [mx, my] = interp.interpolateMarker(Point {12, 10}, 0.5f);
    EXPECT_FLOAT_EQ(mx, 12.0f);
    EXPECT_FLOAT_EQ(my, 10.0f);
}
