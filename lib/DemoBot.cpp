#include "DemoBot.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace qix {

void DemoBot::reset() noexcept
{
    m_state = State::BorderPatrol;
    m_borderDir = Direction::Right;
    m_inwardDir = Direction::Up;
    m_parallelDir = Direction::Right;
    m_returnDir = Direction::Down;
    m_escapeDir = Direction::None;
    m_stepCount = 0;
    m_targetSteps = 0;
    m_patrolTicks = 0;
    m_stuckTicks = 0;
    m_lastMarkerPos = Point {0, 0};
    m_useSlowDraw = false;
}

Point DemoBot::stepPoint(Point p, Direction dir) noexcept
{
    switch (dir) {
    case Direction::Up:
        return Point {p.x, p.y - 1};
    case Direction::Down:
        return Point {p.x, p.y + 1};
    case Direction::Left:
        return Point {p.x - 1, p.y};
    case Direction::Right:
        return Point {p.x + 1, p.y};
    default:
        return p;
    }
}

Direction DemoBot::oppositeDir(Direction dir) noexcept
{
    switch (dir) {
    case Direction::Up:
        return Direction::Down;
    case Direction::Down:
        return Direction::Up;
    case Direction::Left:
        return Direction::Right;
    case Direction::Right:
        return Direction::Left;
    default:
        return Direction::None;
    }
}

bool DemoBot::isCellEmpty(const Playfield* playfield, Point p) const noexcept
{
    if (playfield == nullptr || !playfield->isInBounds(p.x, p.y)) {
        return false;
    }
    return playfield->getCell(p.x, p.y) == CellState::Empty;
}

bool DemoBot::isCellBorder(const Playfield* playfield, Point p, GameMode mode) const noexcept
{
    if (playfield == nullptr || !playfield->isInBounds(p.x, p.y)) {
        return false;
    }
    const auto state = playfield->getCell(p.x, p.y);
    if (state == CellState::Border) {
        return true;
    }
    if (mode == GameMode::Modern && (state == CellState::ClaimedSlow || state == CellState::ClaimedFast)) {
        return true;
    }
    return false;
}

std::int32_t DemoBot::getDistanceToBoundary(const Playfield* playfield, Point p, Direction dir, GameMode mode) noexcept
{
    if (playfield == nullptr || dir == Direction::None) {
        return 9999;
    }

    Point curr = p;
    std::int32_t dist = 0;
    while (dist < 200) {
        curr = stepPoint(curr, dir);
        ++dist;
        if (!playfield->isInBounds(curr.x, curr.y)) {
            return 9999;
        }
        const auto cell = playfield->getCell(curr.x, curr.y);
        if (cell == CellState::Border
            || (mode == GameMode::Modern && (cell == CellState::ClaimedSlow || cell == CellState::ClaimedFast))) {
            return dist;
        }
    }
    return 9999;
}

float DemoBot::getQixDistance(const GameView& view, Point p) const noexcept
{
    float minDistanceSq = 1e9f;
    for (const auto& ribbon : view.qixRibbons) {
        if (ribbon.empty()) {
            continue;
        }
        const auto& head = ribbon.front();
        const float dx1 = static_cast<float>(head.start.x - p.x);
        const float dy1 = static_cast<float>(head.start.y - p.y);
        const float d1 = dx1 * dx1 + dy1 * dy1;
        if (d1 < minDistanceSq) {
            minDistanceSq = d1;
        }
        const float dx2 = static_cast<float>(head.end.x - p.x);
        const float dy2 = static_cast<float>(head.end.y - p.y);
        const float d2 = dx2 * dx2 + dy2 * dy2;
        if (d2 < minDistanceSq) {
            minDistanceSq = d2;
        }
    }
    return (minDistanceSq < 1e8f) ? std::sqrt(minDistanceSq) : 999.0f;
}

bool DemoBot::isQixThreatening(const GameView& view, Point p, float dangerRadius) const noexcept
{
    return getQixDistance(view, p) <= dangerRadius;
}

bool DemoBot::isSparxNear(const GameView& view, Point p, float radius) const noexcept
{
    const float radiusSq = radius * radius;
    for (const auto& sparxPos : view.sparxPositions) {
        const float dx = static_cast<float>(sparxPos.x - p.x);
        const float dy = static_cast<float>(sparxPos.y - p.y);
        if (dx * dx + dy * dy <= radiusSq) {
            return true;
        }
    }
    return false;
}

Direction DemoBot::findQuickestSafeExit(
    const GameView& view, Point marker, const std::vector<Point>& trail) const noexcept
{
    const auto* pf = view.playfield;
    if (pf == nullptr) {
        return Direction::None;
    }

    const std::array<Direction, 4> candidates {Direction::Up, Direction::Down, Direction::Left, Direction::Right};
    Direction bestDir = Direction::None;
    float bestScore = 1e9f;

    for (const auto dir : candidates) {
        const auto nextPt = stepPoint(marker, dir);
        if (!pf->isInBounds(nextPt.x, nextPt.y)) {
            continue;
        }
        // Disallow self-intersection with current trail
        if (std::find(trail.begin(), trail.end(), nextPt) != trail.end()) {
            continue;
        }

        const auto dist = getDistanceToBoundary(pf, marker, dir, view.mode);
        if (dist >= 9999) {
            continue;
        }

        const float qixDist = getQixDistance(view, nextPt);
        // Minimize distance to boundary while maximizing distance from Qix
        const float score = static_cast<float>(dist) * 2.5f - qixDist;
        if (score < bestScore) {
            bestScore = score;
            bestDir = dir;
        }
    }

    return bestDir;
}

PlayerCommand DemoBot::update(const GameView& view) noexcept
{
    if (view.playfield == nullptr) {
        return PlayerCommand {Direction::None, DrawMode::None};
    }

    const auto* pf = view.playfield;
    const auto marker = view.markerPos;

    // Track position changes to detect stalls
    if (marker == m_lastMarkerPos) {
        ++m_stuckTicks;
    } else {
        m_stuckTicks = 0;
        m_lastMarkerPos = marker;
    }

    // 1. Not drawing: navigating perimeter borders
    if (view.drawMode == DrawMode::None) {
        m_state = State::BorderPatrol;
        m_escapeDir = Direction::None;
        ++m_patrolTicks;

        // Check Sparx proximity on the border (evade if within 6 units)
        if (isSparxNear(view, marker, 6.0f)) {
            m_borderDir = oppositeDir(m_borderDir);
        }

        const std::array<Direction, 4> candidates {Direction::Up, Direction::Down, Direction::Left, Direction::Right};

        // Evaluate candidate inward cut directions
        Direction bestInward {Direction::None};
        std::int32_t bestOppositeDist {0};
        float bestCutScore {-1e9f};

        for (const auto dir : candidates) {
            const auto neighbor = stepPoint(marker, dir);
            if (!isCellEmpty(pf, neighbor)) {
                continue;
            }

            const float qixDist = getQixDistance(view, neighbor);
            // Require safe breathing room from Qix before stepping off border
            if (qixDist < 14.0f) {
                continue;
            }

            const auto distOpposite = getDistanceToBoundary(pf, marker, dir, view.mode);
            if (distOpposite >= 9999 || distOpposite <= 2) {
                continue;
            }

            // High score for safe, manageable cuts
            float score = qixDist;
            if (distOpposite <= 14) {
                score += 30.0f; // Bonus for narrow straight slices
            }

            if (score > bestCutScore) {
                bestCutScore = score;
                bestInward = dir;
                bestOppositeDist = distOpposite;
            }
        }

        // Initiate a cut if patrol duration is sufficient and Sparx is clear
        if (bestInward != Direction::None && m_patrolTicks >= 8U && !isSparxNear(view, marker, 8.0f)) {
            m_inwardDir = bestInward;
            m_stepCount = 0;
            m_patrolTicks = 0;

            const float qixDist = getQixDistance(view, marker);
            // Demonstrate Slow Draw (2x score) when Qix is on the far half of the board
            m_useSlowDraw = (qixDist >= 36.0f);

            if (bestOppositeDist <= 14 && qixDist >= 22.0f) {
                // Tactical partition: slice straight across to the opposite boundary
                m_state = State::CuttingStraight;
                m_targetSteps = static_cast<std::uint32_t>(bestOppositeDist);
            } else {
                // Adaptive rectangular cut: scale cut depth to distance and Qix proximity
                m_state = State::CuttingInward;
                const std::int32_t maxSafeDepth = (qixDist >= 32.0f) ? 12 : 6;
                m_targetSteps = static_cast<std::uint32_t>(std::clamp(bestOppositeDist / 3, 4, maxSafeDepth));

                // Determine perpendicular parallel direction
                if (m_inwardDir == Direction::Up || m_inwardDir == Direction::Down) {
                    m_parallelDir = (marker.x < pf->getWidth() / 2) ? Direction::Right : Direction::Left;
                } else {
                    m_parallelDir = (marker.y < pf->getHeight() / 2) ? Direction::Down : Direction::Up;
                }
                m_returnDir = oppositeDir(m_inwardDir);
            }

            const auto drawMode = m_useSlowDraw ? DrawMode::Slow : DrawMode::Fast;
            return PlayerCommand {m_inwardDir, drawMode};
        }

        // Normal border patrol navigation along existing boundary
        auto nextPt = stepPoint(marker, m_borderDir);
        if (!isCellBorder(pf, nextPt, view.mode) || m_stuckTicks >= 2) {
            // Pick an adjacent navigable boundary cell
            for (const auto dir : candidates) {
                if (dir != oppositeDir(m_borderDir)) {
                    const auto pt = stepPoint(marker, dir);
                    if (isCellBorder(pf, pt, view.mode)) {
                        m_borderDir = dir;
                        nextPt = pt;
                        break;
                    }
                }
            }
        }

        return PlayerCommand {m_borderDir, DrawMode::None};
    }

    // 2. Actively drawing Stix line
    const auto activeDraw = (view.drawMode != DrawMode::None) ? view.drawMode : DrawMode::Fast;
    const float qixDist = getQixDistance(view, marker);

    // Emergency evasion: if Qix threatens or bot is stalled, escape to nearest boundary
    // without backtracking onto our own trail (prevents self-intersection freeze bug)
    if (qixDist <= 8.5f || m_stuckTicks >= 2) {
        const auto safeExit = findQuickestSafeExit(view, marker, view.stixTrail);
        if (safeExit != Direction::None) {
            m_state = State::EmergencyEscape;
            m_escapeDir = safeExit;
            return PlayerCommand {m_escapeDir, activeDraw};
        }
    }

    if (m_state == State::EmergencyEscape) {
        const auto nextPt = stepPoint(marker, m_escapeDir);
        if (isCellBorder(pf, nextPt, view.mode)) {
            return PlayerCommand {m_escapeDir, activeDraw};
        }
        // Verify escape direction is still clear of self-intersection
        const auto& trail = view.stixTrail;
        if (!pf->isInBounds(nextPt.x, nextPt.y) || std::find(trail.begin(), trail.end(), nextPt) != trail.end()) {
            m_escapeDir = findQuickestSafeExit(view, marker, trail);
        }
        return PlayerCommand {m_escapeDir, activeDraw};
    }

    switch (m_state) {
    case State::CuttingStraight: {
        // Continue advancing straight until touching the opposing boundary
        return PlayerCommand {m_inwardDir, activeDraw};
    }

    case State::CuttingInward: {
        ++m_stepCount;
        const auto nextPt = stepPoint(marker, m_inwardDir);
        if (m_stepCount >= m_targetSteps || !isCellEmpty(pf, nextPt)) {
            m_state = State::CuttingParallel;
            m_stepCount = 0;
            m_targetSteps = 8U;
            return PlayerCommand {m_parallelDir, activeDraw};
        }
        return PlayerCommand {m_inwardDir, activeDraw};
    }

    case State::CuttingParallel: {
        ++m_stepCount;
        const auto nextPt = stepPoint(marker, m_parallelDir);
        if (m_stepCount >= m_targetSteps || !isCellEmpty(pf, nextPt)) {
            m_state = State::CuttingReturn;
            m_stepCount = 0;
            return PlayerCommand {m_returnDir, activeDraw};
        }
        return PlayerCommand {m_parallelDir, activeDraw};
    }

    case State::CuttingReturn:
    default: {
        const auto nextPt = stepPoint(marker, m_returnDir);
        // Sparx touchdown avoidance: if a Sparx is hovering right at our closure cell,
        // side-step along a clear perpendicular direction to enter safely
        if (isCellBorder(pf, nextPt, view.mode) && isSparxNear(view, nextPt, 3.5f)) {
            const auto sidePt = stepPoint(marker, m_parallelDir);
            if (isCellEmpty(pf, sidePt)) {
                return PlayerCommand {m_parallelDir, activeDraw};
            }
        }
        return PlayerCommand {m_returnDir, activeDraw};
    }
    }
}

} // namespace qix
