#include "Qix.h"
#include <algorithm>
#include <cmath>

namespace qix {

Qix::Qix(LineSegment startLine, std::size_t historyLength) noexcept
    : m_maxHistory {historyLength}
    , m_p1 {startLine.start}
    , m_p2 {startLine.end}
{
    if (m_maxHistory < 2) {
        m_maxHistory = 2;
    }

    m_segments.push_back(startLine);
}

void Qix::update(const Playfield& field) noexcept
{
    // Advance and bounce both endpoints independently
    bounceEndpoint(m_p1, m_vx1, m_vy1, field);
    bounceEndpoint(m_p2, m_vx2, m_vy2, field);

    // Record new head segment
    LineSegment newHead {m_p1, m_p2};
    m_segments.push_front(newHead);

    // Maintain ribbon length
    while (m_segments.size() > m_maxHistory) {
        m_segments.pop_back();
    }
}

const std::deque<LineSegment>& Qix::getSegments() const noexcept
{
    return m_segments;
}

LineSegment Qix::getHead() const noexcept
{
    return m_segments.empty() ? LineSegment {m_p1, m_p2} : m_segments.front();
}

bool Qix::intersects(Point pt) const noexcept
{
    for (const auto& seg : m_segments) {
        if (isPointOnSegment(pt, seg.start, seg.end)) {
            return true;
        }
    }

    return false;
}

bool Qix::intersectsTrail(const std::vector<Point>& trail) const noexcept
{
    for (const auto& pt : trail) {
        if (intersects(pt)) {
            return true;
        }
    }

    return false;
}

void Qix::bounceEndpoint(Point& p, std::int32_t& vx, std::int32_t& vy, const Playfield& field) noexcept
{
    Point nextX {p.x + vx, p.y};
    if (!field.isInBounds(nextX.x, nextX.y) || field.getCell(nextX.x, nextX.y) != CellState::Empty) {
        vx = -vx;
    }

    Point nextY {p.x, p.y + vy};
    if (!field.isInBounds(nextY.x, nextY.y) || field.getCell(nextY.x, nextY.y) != CellState::Empty) {
        vy = -vy;
    }

    Point nextFull {p.x + vx, p.y + vy};
    if (!field.isInBounds(nextFull.x, nextFull.y) || field.getCell(nextFull.x, nextFull.y) != CellState::Empty) {
        vx = -vx;
        vy = -vy;
    }

    p.x += vx;
    p.y += vy;

    // Keep point constrained inside playfield
    if (p.x < 1) {
        p.x = 1;
        vx = std::abs(vx);
    } else if (p.x >= field.getWidth() - 1) {
        p.x = field.getWidth() - 2;
        vx = -std::abs(vx);
    }

    if (p.y < 1) {
        p.y = 1;
        vy = std::abs(vy);
    } else if (p.y >= field.getHeight() - 1) {
        p.y = field.getHeight() - 2;
        vy = -std::abs(vy);
    }
}

bool Qix::isPointOnSegment(Point p, Point a, Point b) noexcept
{
    // Check bounding box with 1-cell tolerance
    const auto minX = std::min(a.x, b.x) - 1;
    const auto maxX = std::max(a.x, b.x) + 1;
    const auto minY = std::min(a.y, b.y) - 1;
    const auto maxY = std::max(a.y, b.y) + 1;

    if (p.x < minX || p.x > maxX || p.y < minY || p.y > maxY) {
        return false;
    }

    // Distance from point to line segment via cross product area
    const auto dx = static_cast<double>(b.x - a.x);
    const auto dy = static_cast<double>(b.y - a.y);
    const auto lengthSq = dx * dx + dy * dy;

    if (lengthSq < 1e-6) {
        return (std::abs(p.x - a.x) <= 1) && (std::abs(p.y - a.y) <= 1);
    }

    // Area of triangle / base length
    const auto numerator = std::abs(
        dy * static_cast<double>(p.x) - dx * static_cast<double>(p.y) + static_cast<double>(b.x * a.y - b.y * a.x));
    const auto distance = numerator / std::sqrt(lengthSq);

    return distance <= 1.5;
}

} // namespace qix
