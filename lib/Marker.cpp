#include "Marker.h"
#include <algorithm>

namespace qix {

Marker::Marker(Point startPos, std::uint8_t lives) noexcept
    : m_position {startPos}
    , m_lives {lives}
{
}

void Marker::resetPosition(Point resetPos) noexcept
{
    m_position = resetPos;
    m_drawMode = DrawMode::None;
    m_trail.clear();
}

bool Marker::move(Playfield& field, PlayerCommand cmd) noexcept
{
    if (cmd.direction == Direction::None) {
        return false;
    }

    const auto nextPos = calculateNext(m_position, cmd.direction);
    if (!field.isInBounds(nextPos.x, nextPos.y)) {
        return false;
    }

    const auto nextCell = field.getCell(nextPos.x, nextPos.y);

    // Case 1: Marker is currently navigating along existing perimeter
    if (!isDrawing()) {
        if (cmd.drawMode == DrawMode::None) {
            // Can only traverse along borders or claimed regions when not drawing
            if (!isBorderOrClaimed(nextCell)) {
                return false;
            }

            m_position = nextPos;
            return true;
        }

        // Commencing a new Stix into empty space
        if (nextCell != CellState::Empty) {
            // If drawing button held but moving along border, treat as border move
            if (isBorderOrClaimed(nextCell)) {
                m_position = nextPos;
                return true;
            }
            return false;
        }

        m_drawMode = cmd.drawMode;
        m_trail.push_back(m_position);
        m_position = nextPos;
        m_trail.push_back(m_position);
        field.setCell(nextPos.x, nextPos.y, CellState::ActiveStix);
        return true;
    }

    // Case 2: Marker is actively drawing a Stix line
    // Disallow self-intersection with current trail
    const auto hitTrail = std::find(m_trail.begin(), m_trail.end(), nextPos);
    if (hitTrail != m_trail.end()) {
        return false;
    }

    // Entering empty territory: advance trail
    if (nextCell == CellState::Empty) {
        m_position = nextPos;
        m_trail.push_back(m_position);
        field.setCell(nextPos.x, nextPos.y, CellState::ActiveStix);
        return true;
    }

    // Reached boundary or claimed territory: loop closure
    if (isBorderOrClaimed(nextCell)) {
        m_position = nextPos;
        m_trail.push_back(m_position);
        return true;
    }

    return false;
}

bool Marker::isDrawing() const noexcept
{
    return m_drawMode != DrawMode::None;
}

DrawMode Marker::getDrawMode() const noexcept
{
    return m_drawMode;
}

Point Marker::getPosition() const noexcept
{
    return m_position;
}

const std::vector<Point>& Marker::getTrail() const noexcept
{
    return m_trail;
}

void Marker::clearTrail() noexcept
{
    m_trail.clear();
    m_drawMode = DrawMode::None;
}

std::uint8_t Marker::getLives() const noexcept
{
    return m_lives;
}

void Marker::decrementLives() noexcept
{
    if (m_lives > 0) {
        --m_lives;
    }
}

bool Marker::isAlive() const noexcept
{
    return m_lives > 0;
}

Point Marker::calculateNext(Point current, Direction dir) noexcept
{
    switch (dir) {
    case Direction::Up:
        return Point {current.x, current.y - 1};
    case Direction::Down:
        return Point {current.x, current.y + 1};
    case Direction::Left:
        return Point {current.x - 1, current.y};
    case Direction::Right:
        return Point {current.x + 1, current.y};
    case Direction::None:
    default:
        return current;
    }
}

bool Marker::isBorderOrClaimed(CellState state) noexcept
{
    return state == CellState::Border || state == CellState::ClaimedSlow || state == CellState::ClaimedFast;
}

} // namespace qix
