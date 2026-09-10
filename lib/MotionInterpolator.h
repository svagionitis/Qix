#pragma once

#include "IQixGame.h"
#include "Types.h"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <utility>
#include <vector>

namespace qix {

/// @class MotionInterpolator
/// @brief Sub-pixel entity motion interpolator between discrete simulation ticks.
/// @details Calculates the normalized interpolation factor:
/// \f[
/// \alpha = \frac{t_{\text{current}} - t_{\text{lastTick}}}{\Delta t_{\text{tick}}}
/// \f]
/// and linearly interpolates Marker and Sparx coordinates between the previous tick and the current tick
/// for analog-smooth rendering on high-refresh-rate displays (60 Hz, 120 Hz, 144 Hz, 240 Hz).
class MotionInterpolator {
public:
    MotionInterpolator() = default;
    ~MotionInterpolator() = default;

    MotionInterpolator(const MotionInterpolator&) = default;
    MotionInterpolator& operator=(const MotionInterpolator&) = default;
    MotionInterpolator(MotionInterpolator&&) noexcept = default;
    MotionInterpolator& operator=(MotionInterpolator&&) noexcept = default;

    /// @brief Calculate normalized interpolation alpha factor in [0.0f, 1.0f].
    /// @param[in] elapsedSeconds Time elapsed in seconds since the last simulation tick.
    /// @param[in] stepIntervalSeconds Duration of one simulation tick in seconds.
    /// @return Interpolation factor clamped to [0.0f, 1.0f].
    [[nodiscard]] static float calculateAlpha(double elapsedSeconds, double stepIntervalSeconds) noexcept
    {
        if (stepIntervalSeconds <= 0.0) {
            return 1.0f;
        }
        const double alpha = elapsedSeconds / stepIntervalSeconds;
        return static_cast<float>(std::clamp(alpha, 0.0, 1.0));
    }

    /// @brief Record entity positions before a simulation step.
    /// @param[in] view Current game view before advancing the simulation.
    void onTick(const GameView& view) noexcept
    {
        m_prevMarkerPos = view.markerPos;
        m_hasPrevMarker = true;

        m_prevSparxPositions.clear();
        if (!view.sparxList.empty()) {
            m_prevSparxPositions.reserve(view.sparxList.size());
            for (const auto& sp : view.sparxList) {
                m_prevSparxPositions.push_back(sp.position);
            }
        } else {
            m_prevSparxPositions = view.sparxPositions;
        }
    }

    /// @brief Reset interpolation state to prevent interpolation across state discontinuities.
    void reset() noexcept
    {
        m_hasPrevMarker = false;
        m_prevMarkerPos = Point {0, 0};
        m_prevSparxPositions.clear();
    }

    /// @brief Check whether valid previous entity positions are currently recorded.
    /// @return True if previous positions exist.
    [[nodiscard]] bool hasPreviousState() const noexcept
    {
        return m_hasPrevMarker;
    }

    /// @brief Retrieve recorded previous marker position.
    /// @return Previous marker Point.
    [[nodiscard]] Point getPreviousMarker() const noexcept
    {
        return m_prevMarkerPos;
    }

    /// @brief Retrieve recorded previous Sparx positions.
    /// @return Const reference to vector of previous Sparx Points.
    [[nodiscard]] const std::vector<Point>& getPreviousSparx() const noexcept
    {
        return m_prevSparxPositions;
    }

    /// @brief Compute linearly interpolated grid position for the player marker.
    /// @param[in] current Current discrete grid coordinates of marker.
    /// @param[in] alpha Interpolation factor in [0.0f, 1.0f].
    /// @return Pair of floating-point (x, y) coordinates in grid space.
    [[nodiscard]] std::pair<float, float> interpolateMarker(Point current, float alpha) const noexcept
    {
        if (!m_hasPrevMarker || alpha >= 1.0f) {
            return {static_cast<float>(current.x), static_cast<float>(current.y)};
        }

        const int dx = current.x - m_prevMarkerPos.x;
        const int dy = current.y - m_prevMarkerPos.y;

        // Reject discontinuous jumps (e.g. death respawn, teleport, level change)
        if (std::abs(dx) <= 1 && std::abs(dy) <= 1) {
            return {
                static_cast<float>(m_prevMarkerPos.x) + static_cast<float>(dx) * alpha,
                static_cast<float>(m_prevMarkerPos.y) + static_cast<float>(dy) * alpha,
            };
        }

        return {static_cast<float>(current.x), static_cast<float>(current.y)};
    }

    /// @brief Compute linearly interpolated grid position for a Sparx entity by index.
    /// @param[in] index Index of Sparx in entity list.
    /// @param[in] current Current discrete grid coordinates of Sparx.
    /// @param[in] alpha Interpolation factor in [0.0f, 1.0f].
    /// @return Pair of floating-point (x, y) coordinates in grid space.
    [[nodiscard]] std::pair<float, float> interpolateSparx(std::size_t index, Point current, float alpha) const noexcept
    {
        if (index >= m_prevSparxPositions.size() || alpha >= 1.0f) {
            return {static_cast<float>(current.x), static_cast<float>(current.y)};
        }

        const auto prev = m_prevSparxPositions[index];
        const int dx = current.x - prev.x;
        const int dy = current.y - prev.y;

        // Sparx moves at most 1 (normal) or 2 (super/escalation) steps per tick
        if (std::abs(dx) <= 2 && std::abs(dy) <= 2) {
            return {
                static_cast<float>(prev.x) + static_cast<float>(dx) * alpha,
                static_cast<float>(prev.y) + static_cast<float>(dy) * alpha,
            };
        }

        return {static_cast<float>(current.x), static_cast<float>(current.y)};
    }

private:
    bool m_hasPrevMarker {false};
    Point m_prevMarkerPos {0, 0};
    std::vector<Point> m_prevSparxPositions {};
};

} // namespace qix
