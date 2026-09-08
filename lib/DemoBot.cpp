#include "DemoBot.h"
#include <cmath>

namespace qix {

void DemoBot::reset() noexcept
{
    m_state = State::BorderPatrol;
    m_borderDir = Direction::Right;
    m_inwardDir = Direction::Up;
    m_parallelDir = Direction::Right;
    m_returnDir = Direction::Down;
    m_stepCount = 0;
    m_targetSteps = 0;
    m_patrolTicks = 0;
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

bool DemoBot::isQixThreatening(const GameView& view, Point p, float dangerRadius) const noexcept
{
    const float radiusSq = dangerRadius * dangerRadius;
    for (const auto& ribbon : view.qixRibbons) {
        if (ribbon.empty()) {
            continue;
        }
        // Inspect head segment
        const auto& head = ribbon.front();
        const float dx1 = static_cast<float>(head.start.x - p.x);
        const float dy1 = static_cast<float>(head.start.y - p.y);
        if (dx1 * dx1 + dy1 * dy1 <= radiusSq) {
            return true;
        }
        const float dx2 = static_cast<float>(head.end.x - p.x);
        const float dy2 = static_cast<float>(head.end.y - p.y);
        if (dx2 * dx2 + dy2 * dy2 <= radiusSq) {
            return true;
        }
    }
    return false;
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

PlayerCommand DemoBot::update(const GameView& view) noexcept
{
    if (view.playfield == nullptr) {
        return PlayerCommand {Direction::None, DrawMode::None};
    }

    const auto* pf = view.playfield;
    const auto marker = view.markerPos;

    // 1. If not actively drawing, we are patrolling the border
    if (view.drawMode == DrawMode::None) {
        m_state = State::BorderPatrol;
        ++m_patrolTicks;

        // Check Sparx proximity on the border
        if (isSparxNear(view, marker, 6.0f)) {
            // Reverse direction to evade Sparx
            m_borderDir = oppositeDir(m_borderDir);
        }

        // Test if an inward cut is viable
        const std::array<Direction, 4> candidates {Direction::Up, Direction::Down, Direction::Left, Direction::Right};
        Direction viableInward {Direction::None};

        for (const auto dir : candidates) {
            const auto neighbor = stepPoint(marker, dir);
            if (isCellEmpty(pf, neighbor)) {
                // Check if Qix is far enough away from this entry point
                if (!isQixThreatening(view, neighbor, 16.0f)) {
                    viableInward = dir;
                    break;
                }
            }
        }

        // Initiate cut if we have patrolled enough and found a safe entry
        if (viableInward != Direction::None && m_patrolTicks >= 12U && !isSparxNear(view, marker, 8.0f)) {
            m_inwardDir = viableInward;
            m_state = State::CuttingInward;
            m_stepCount = 0;
            m_targetSteps = 12U;
            m_patrolTicks = 0;
            m_useSlowDraw = !m_useSlowDraw; // Alternate drawing speed

            // Determine perpendicular parallel direction
            if (m_inwardDir == Direction::Up || m_inwardDir == Direction::Down) {
                m_parallelDir = (marker.x < pf->getWidth() / 2) ? Direction::Right : Direction::Left;
            } else {
                m_parallelDir = (marker.y < pf->getHeight() / 2) ? Direction::Down : Direction::Up;
            }
            m_returnDir = oppositeDir(m_inwardDir);

            const auto drawMode = m_useSlowDraw ? DrawMode::Slow : DrawMode::Fast;
            return PlayerCommand {m_inwardDir, drawMode};
        }

        // Normal border patrol movement
        auto nextPt = stepPoint(marker, m_borderDir);
        if (!isCellBorder(pf, nextPt, view.mode)) {
            // Find a valid adjacent border cell to turn along perimeter
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

    // 2. Active Stix drawing in progress
    const auto activeDraw = (view.drawMode != DrawMode::None) ? view.drawMode : DrawMode::Fast;

    // Emergency evasion: if Qix is dangerously close, immediately steer to return border
    if (isQixThreatening(view, marker, 10.0f)) {
        m_state = State::CuttingReturn;
        return PlayerCommand {m_returnDir, activeDraw};
    }

    switch (m_state) {
    case State::CuttingInward: {
        ++m_stepCount;
        const auto nextPt = stepPoint(marker, m_inwardDir);
        if (m_stepCount >= m_targetSteps || !isCellEmpty(pf, nextPt)) {
            m_state = State::CuttingParallel;
            m_stepCount = 0;
            m_targetSteps = 10U;
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
        // Keep returning until border is reached and cut completes
        return PlayerCommand {m_returnDir, activeDraw};
    }
    }
}

} // namespace qix
