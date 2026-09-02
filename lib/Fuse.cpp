#include "Fuse.h"

namespace qix {

Fuse::Fuse(std::uint32_t idleTicksToIgnite) noexcept
    : m_idleLimit {idleTicksToIgnite}
{
}

void Fuse::update(bool isDrawing, bool markerMoved, const std::vector<Point>& trail) noexcept
{
    // Extinguish fuse if player is not drawing
    if (!isDrawing || trail.empty()) {
        reset();
        return;
    }

    // Advance idle counter if stationary while drawing
    if (!markerMoved) {
        ++m_idleCounter;
        if (m_idleCounter >= m_idleLimit) {
            m_burning = true;
        }
    } else if (!m_burning) {
        // Reset idle counter if moving and not yet ignited
        m_idleCounter = 0;
    }

    // Advance fuse along trail if burning
    if (m_burning) {
        if (m_trailIndex < trail.size()) {
            m_position = trail[m_trailIndex];
            ++m_trailIndex;
        } else if (!trail.empty()) {
            m_position = trail.back();
        }
    }
}

void Fuse::reset() noexcept
{
    m_burning = false;
    m_idleCounter = 0;
    m_trailIndex = 0;
    m_position = Point {0, 0};
}

bool Fuse::isBurning() const noexcept
{
    return m_burning;
}

std::optional<Point> Fuse::getPosition() const noexcept
{
    if (!m_burning) {
        return std::nullopt;
    }

    return m_position;
}

bool Fuse::checkCollision(Point markerPos) const noexcept
{
    if (!m_burning) {
        return false;
    }

    return m_position == markerPos;
}

} // namespace qix
