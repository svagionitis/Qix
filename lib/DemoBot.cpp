#include "DemoBot.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace qix {

namespace {

    [[nodiscard]] float distSqPointToSegment(Point p, Point a, Point b) noexcept
    {
        const float px = static_cast<float>(p.x);
        const float py = static_cast<float>(p.y);
        const float ax = static_cast<float>(a.x);
        const float ay = static_cast<float>(a.y);
        const float bx = static_cast<float>(b.x);
        const float by = static_cast<float>(b.y);

        const float abx = bx - ax;
        const float aby = by - ay;
        const float apx = px - ax;
        const float apy = py - ay;

        const float abLenSq = abx * abx + aby * aby;
        if (abLenSq < 1e-4f) {
            return apx * apx + apy * apy;
        }

        const float t = std::clamp((apx * abx + apy * aby) / abLenSq, 0.0f, 1.0f);
        const float projX = ax + t * abx;
        const float projY = ay + t * aby;

        const float dx = px - projX;
        const float dy = py - projY;
        return dx * dx + dy * dy;
    }

    [[nodiscard]] float euclideanDist(Point a, Point b) noexcept
    {
        const float dx = static_cast<float>(a.x - b.x);
        const float dy = static_cast<float>(a.y - b.y);
        return std::sqrt(dx * dx + dy * dy);
    }

} // namespace

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
    // Check all segments across all Qix ribbons (full body coverage, not just head)
    for (const auto& ribbon : view.qixRibbons) {
        for (const auto& seg : ribbon) {
            const float dSq = distSqPointToSegment(p, seg.start, seg.end);
            if (dSq < minDistanceSq) {
                minDistanceSq = dSq;
            }
        }
    }
    return (minDistanceSq < 1e8f) ? std::sqrt(minDistanceSq) : 999.0f;
}

float DemoBot::getMinQixDistanceToTrail(const GameView& view, const std::vector<Point>& trail) const noexcept
{
    if (trail.empty()) {
        return 999.0f;
    }
    float minTrailDist = 999.0f;
    for (const auto& pt : trail) {
        const float d = getQixDistance(view, pt);
        if (d < minTrailDist) {
            minTrailDist = d;
        }
    }
    return minTrailDist;
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
        // Heavily prioritize shortest distance to close immediately, with Qix distance buffer
        const float score = static_cast<float>(dist) * 3.0f - qixDist;
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

        const std::array<Direction, 4> candidates {Direction::Up, Direction::Down, Direction::Left, Direction::Right};

        // Intelligent Sparx evasion on border: check if stepping forward reduces distance to any Sparx
        for (const auto& sparxPos : view.sparxPositions) {
            const float d = euclideanDist(marker, sparxPos);
            if (d <= 8.0f) {
                const auto nextPt = stepPoint(marker, m_borderDir);
                const float nextD = euclideanDist(nextPt, sparxPos);
                if (nextD < d) {
                    // Moving towards Sparx: reverse direction
                    m_borderDir = oppositeDir(m_borderDir);
                    break;
                }
            }
        }

        // Evaluate candidate inward cut directions with strict safety requirements
        Direction bestInward {Direction::None};
        std::int32_t bestOppositeDist {0};
        std::int32_t bestPerpDist {9999};
        Direction bestPerpDir {Direction::None};
        float bestCutScore {-1e9f};

        for (const auto dir : candidates) {
            const auto neighbor = stepPoint(marker, dir);
            if (!isCellEmpty(pf, neighbor)) {
                continue;
            }

            const float qixDist = getQixDistance(view, neighbor);
            // Safe threshold: cut takes ~8-11 steps, so 15.0f ensures Qix cannot intercept
            if (qixDist < 15.0f) {
                continue;
            }

            // Do not leave border if any Sparx is dangerously close (<= 4.5 units)
            if (isSparxNear(view, marker, 4.5f)) {
                continue;
            }

            const auto distOpposite = getDistanceToBoundary(pf, marker, dir, view.mode);
            if (distOpposite >= 9999 || distOpposite <= 2) {
                continue;
            }

            // Check perpendicular distances across empty space from the inward neighbor
            Direction pDir1 = (dir == Direction::Up || dir == Direction::Down) ? Direction::Left : Direction::Up;
            Direction pDir2 = (dir == Direction::Up || dir == Direction::Down) ? Direction::Right : Direction::Down;
            const auto dP1 = getDistanceToBoundary(pf, neighbor, pDir1, view.mode);
            const auto dP2 = getDistanceToBoundary(pf, neighbor, pDir2, view.mode);
            Direction closerPerp = (dP1 < dP2) ? pDir1 : pDir2;
            std::int32_t minPerpD = std::min(dP1, dP2);

            float score = qixDist;
            if (minPerpD <= 8) {
                score += 60.0f; // High priority for compact L-cuts near corners
            } else if (distOpposite <= 10) {
                score += 40.0f; // Priority for narrow channel slices
            }

            if (score > bestCutScore) {
                bestCutScore = score;
                bestInward = dir;
                bestOppositeDist = distOpposite;
                bestPerpDist = minPerpD;
                bestPerpDir = closerPerp;
            }
        }

        // Initiate cut if patrolled long enough and conditions are safe
        if (bestInward != Direction::None && m_patrolTicks >= 4U && !isSparxNear(view, marker, 4.5f)) {
            m_inwardDir = bestInward;
            m_stepCount = 0;
            m_patrolTicks = 0;

            const float qixDist = getQixDistance(view, marker);
            // Only use Slow Draw for tiny cuts when Qix is very far (> 38 units)
            m_useSlowDraw = (qixDist >= 38.0f && (bestPerpDist <= 5 || bestOppositeDist <= 6));

            if (bestPerpDist <= 8 && bestPerpDir != Direction::None) {
                // Highly safe 2-segment L-cut to adjacent border
                m_state = State::CuttingInward;
                m_targetSteps = static_cast<std::uint32_t>(std::clamp(bestPerpDist / 2, 3, 5));
                m_parallelDir = bestPerpDir;
                m_returnDir = bestPerpDir; // Completes directly on adjacent border!
            } else if (bestOppositeDist <= 10 && qixDist >= 20.0f) {
                // Straight channel partition
                m_state = State::CuttingStraight;
                m_targetSteps = static_cast<std::uint32_t>(bestOppositeDist);
            } else {
                // Compact U-cut nibble: depth 3, parallel 4 away from Qix
                m_state = State::CuttingInward;
                m_targetSteps = 3U;

                Point qixPos {pf->getWidth() / 2, pf->getHeight() / 2};
                if (!view.qixRibbons.empty() && !view.qixRibbons[0].empty()) {
                    qixPos = view.qixRibbons[0].front().start;
                }
                if (m_inwardDir == Direction::Up || m_inwardDir == Direction::Down) {
                    m_parallelDir = (qixPos.x > marker.x) ? Direction::Left : Direction::Right;
                } else {
                    m_parallelDir = (qixPos.y > marker.y) ? Direction::Up : Direction::Down;
                }
                m_returnDir = oppositeDir(m_inwardDir);
            }

            const auto drawMode = m_useSlowDraw ? DrawMode::Slow : DrawMode::Fast;
            return PlayerCommand {m_inwardDir, drawMode};
        }

        // Normal border patrol navigation along existing boundary
        auto nextPt = stepPoint(marker, m_borderDir);
        if (!isCellBorder(pf, nextPt, view.mode) || m_stuckTicks >= 2) {
            // Pick the adjacent navigable boundary cell that maximizes distance to nearest Sparx
            Direction bestTurn = Direction::None;
            float bestSparxDist = -1.0f;

            for (const auto dir : candidates) {
                if (dir != oppositeDir(m_borderDir)) {
                    const auto pt = stepPoint(marker, dir);
                    if (isCellBorder(pf, pt, view.mode)) {
                        float minD = 999.0f;
                        for (const auto& sparxPos : view.sparxPositions) {
                            minD = std::min(minD, euclideanDist(pt, sparxPos));
                        }
                        if (minD > bestSparxDist) {
                            bestSparxDist = minD;
                            bestTurn = dir;
                        }
                    }
                }
            }
            // Allow 180-degree reverse if dead-end reached
            if (bestTurn == Direction::None) {
                const auto revDir = oppositeDir(m_borderDir);
                const auto revPt = stepPoint(marker, revDir);
                if (isCellBorder(pf, revPt, view.mode)) {
                    bestTurn = revDir;
                }
            }
            if (bestTurn != Direction::None) {
                m_borderDir = bestTurn;
                nextPt = stepPoint(marker, m_borderDir);
            }
        }

        return PlayerCommand {m_borderDir, DrawMode::None};
    }

    // 2. Actively drawing Stix line
    const auto activeDraw = (view.drawMode != DrawMode::None) ? view.drawMode : DrawMode::Fast;
    const float qixMarkerDist = getQixDistance(view, marker);
    const float qixTrailDist = getMinQixDistanceToTrail(view, view.stixTrail);

    // Continuous vigilance: if Qix threatens ANY part of our trail or marker, or if stalled,
    // trigger immediate emergency escape to the closest boundary
    const float minSafety = std::min(qixMarkerDist, qixTrailDist);
    if (minSafety <= 8.5f || m_stuckTicks >= 2) {
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
        return PlayerCommand {m_inwardDir, activeDraw};
    }

    case State::CuttingInward: {
        ++m_stepCount;
        const auto nextPt = stepPoint(marker, m_inwardDir);
        if (m_stepCount >= m_targetSteps || !isCellEmpty(pf, nextPt)) {
            // Transition directly to return if parallel steps not needed
            if (m_returnDir == m_parallelDir) {
                m_state = State::CuttingReturn;
            } else {
                m_state = State::CuttingParallel;
            }
            m_stepCount = 0;
            m_targetSteps = 5U; // Keep parallel segment short and tight
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
        // Sparx touchdown avoidance: if a Sparx is within 3.5 units of closure cell,
        // detour along perpendicular direction to enter safely
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
