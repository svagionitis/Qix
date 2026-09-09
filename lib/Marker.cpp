#include "Marker.h"
#include <algorithm>

namespace qix {

Marker::Marker(Point startPos, std::uint8_t lives, GameMode mode) noexcept
    : m_position {startPos}
    , m_lives {lives}
    , m_mode {mode}
{
}

void Marker::resetPosition(Point resetPos) noexcept
{
    m_position = resetPos;
    m_drawMode = DrawMode::None;
    m_trail.clear();
    m_slowTick = false;
    m_pacingWait = false;
}

bool Marker::move(Playfield& field, PlayerCommand cmd) noexcept
{
    m_pacingWait = false;

    if (cmd.direction == Direction::None) {
        m_slowTick = false;
        return false;
    }

    const auto nextPos = calculateNext(m_position, cmd.direction);
    if (!field.isInBounds(nextPos.x, nextPos.y)) {
        m_slowTick = false;
        return false;
    }

    const auto nextCell = field.getCell(nextPos.x, nextPos.y);

    // Case 1: Marker is currently navigating along existing perimeter
    if (!isDrawing()) {
        m_slowTick = false;
        if (cmd.drawMode == DrawMode::None) {
            // Can only traverse along navigable regions when not drawing
            if (!isNavigable(nextCell)) {
                return false;
            }

            m_position = nextPos;
            return true;
        }

        // Commencing a new Stix into empty space
        if (nextCell != CellState::Empty) {
            // If drawing button held but moving along border/navigable area, treat as border move
            if (isNavigable(nextCell)) {
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
        m_slowTick = (m_drawMode == DrawMode::Slow);
        return true;
    }

    // Case 2: Marker is actively drawing a Stix line
    // Hold-to-draw arcade rule: advancing the active Stix requires holding the matching draw button.
    // Releasing the button (DrawMode::None) or attempting to switch draw modes mid-stroke halts the marker.
    if (cmd.drawMode != m_drawMode) {
        m_slowTick = false;
        return false;
    }

    // Disallow self-intersection with current trail
    const auto hitTrail = std::find(m_trail.begin(), m_trail.end(), nextPos);
    if (hitTrail != m_trail.end()) {
        m_slowTick = false;
        return false;
    }

    // Slow Draw pacing: advance 1 cell every 2 ticks (authentic arcade half-speed)
    if (m_drawMode == DrawMode::Slow && m_slowTick) {
        m_slowTick = false;
        m_pacingWait = true;
        return false;
    }

    // Entering empty territory: advance trail
    if (nextCell == CellState::Empty) {
        m_position = nextPos;
        m_trail.push_back(m_position);
        field.setCell(nextPos.x, nextPos.y, CellState::ActiveStix);
        m_slowTick = (m_drawMode == DrawMode::Slow);
        return true;
    }

    // Reached boundary or claimed territory: loop closure
    if (isNavigable(nextCell)) {
        m_position = nextPos;
        m_trail.push_back(m_position);
        m_slowTick = false;
        return true;
    }

    m_slowTick = false;
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
    m_slowTick = false;
    m_pacingWait = false;
}

std::uint8_t Marker::getLives() const noexcept
{
    return m_lives;
}

void Marker::incrementLives() noexcept
{
    if (m_lives < MaxLives) {
        ++m_lives;
    }
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

GameMode Marker::getGameMode() const noexcept
{
    return m_mode;
}

void Marker::setGameMode(GameMode mode) noexcept
{
    m_mode = mode;
}

bool Marker::isNavigable(CellState state) const noexcept
{
    if (m_mode == GameMode::Classic) {
        return state == CellState::Border;
    }
    return state == CellState::Border || state == CellState::ClaimedSlow || state == CellState::ClaimedFast;
}

bool Marker::isBorderOrClaimed(CellState state) noexcept
{
    return state == CellState::Border || state == CellState::ClaimedSlow || state == CellState::ClaimedFast;
}

bool Marker::isPacingWait() const noexcept
{
    return m_pacingWait;
}

void Marker::restore(Point pos, DrawMode mode, std::uint8_t lives, const std::vector<Point>& trail) noexcept
{
    m_position = pos;
    m_drawMode = mode;
    m_lives = (lives > MaxLives) ? MaxLives : lives;
    m_trail = trail;
    m_slowTick = false;
    m_pacingWait = false;
}

} // namespace qix
