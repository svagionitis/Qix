#include "Sparx.h"
#include <array>

namespace qix {

Sparx::Sparx(Point startPos, bool clockwise, GameMode mode, bool isSuper) noexcept
    : m_position {startPos}
    , m_clockwise {clockwise}
    , m_mode {mode}
    , m_isSuper {isSuper}
{
}

void Sparx::update(const Playfield& field) noexcept
{
    // Define relative turn order based on clockwise vs counter-clockwise
    std::array<Direction, 4> searchDirs {};
    if (m_clockwise) {
        // Clockwise priority: right turn, forward, left turn, reverse
        switch (m_lastDir) {
        case Direction::Up:
            searchDirs = {Direction::Right, Direction::Up, Direction::Left, Direction::Down};
            break;
        case Direction::Right:
            searchDirs = {Direction::Down, Direction::Right, Direction::Up, Direction::Left};
            break;
        case Direction::Down:
            searchDirs = {Direction::Left, Direction::Down, Direction::Right, Direction::Up};
            break;
        case Direction::Left:
            searchDirs = {Direction::Up, Direction::Left, Direction::Down, Direction::Right};
            break;
        case Direction::None:
        default:
            searchDirs = {Direction::Right, Direction::Down, Direction::Left, Direction::Up};
            break;
        }
    } else {
        // Counter-clockwise priority: left turn, forward, right turn, reverse
        switch (m_lastDir) {
        case Direction::Up:
            searchDirs = {Direction::Left, Direction::Up, Direction::Right, Direction::Down};
            break;
        case Direction::Left:
            searchDirs = {Direction::Down, Direction::Left, Direction::Up, Direction::Right};
            break;
        case Direction::Down:
            searchDirs = {Direction::Right, Direction::Down, Direction::Left, Direction::Up};
            break;
        case Direction::Right:
            searchDirs = {Direction::Up, Direction::Right, Direction::Down, Direction::Left};
            break;
        case Direction::None:
        default:
            searchDirs = {Direction::Left, Direction::Down, Direction::Right, Direction::Up};
            break;
        }
    }

    // 1. If Super Sparx, prioritize following active Stix trail in forward/turn directions
    if (m_isSuper) {
        for (std::size_t i {0}; i < 3; ++i) {
            const auto dir = searchDirs[i];
            Point candidate {m_position.x, m_position.y};
            switch (dir) {
            case Direction::Up:
                --candidate.y;
                break;
            case Direction::Down:
                ++candidate.y;
                break;
            case Direction::Left:
                --candidate.x;
                break;
            case Direction::Right:
                ++candidate.x;
                break;
            case Direction::None:
            default:
                break;
            }

            if (field.isInBounds(candidate.x, candidate.y)) {
                if (field.getCell(candidate.x, candidate.y) == CellState::ActiveStix) {
                    m_position = candidate;
                    m_lastDir = dir;
                    return;
                }
            }
        }
    }

    // 2. Standard perimeter traversal
    for (const auto dir : searchDirs) {
        Point candidate {m_position.x, m_position.y};
        switch (dir) {
        case Direction::Up:
            --candidate.y;
            break;
        case Direction::Down:
            ++candidate.y;
            break;
        case Direction::Left:
            --candidate.x;
            break;
        case Direction::Right:
            ++candidate.x;
            break;
        case Direction::None:
        default:
            break;
        }

        if (field.isInBounds(candidate.x, candidate.y)) {
            const auto state = field.getCell(candidate.x, candidate.y);
            if (isPerimeter(state)) {
                m_position = candidate;
                m_lastDir = dir;
                return;
            }
        }
    }
}

Point Sparx::getPosition() const noexcept
{
    return m_position;
}

bool Sparx::checkCollision(Point markerPos) const noexcept
{
    return m_position == markerPos;
}

bool Sparx::isSuper() const noexcept
{
    return m_isSuper;
}

void Sparx::setSuper(bool isSuper) noexcept
{
    m_isSuper = isSuper;
}

bool Sparx::isClockwise() const noexcept
{
    return m_clockwise;
}

void Sparx::restore(Point pos, bool clockwise, bool isSuper) noexcept
{
    m_position = pos;
    m_clockwise = clockwise;
    m_isSuper = isSuper;
    m_lastDir = clockwise ? Direction::Right : Direction::Left;
}

GameMode Sparx::getGameMode() const noexcept
{
    return m_mode;
}

void Sparx::setGameMode(GameMode mode) noexcept
{
    m_mode = mode;
}

bool Sparx::isPerimeter(CellState state) const noexcept
{
    if (m_mode == GameMode::Classic) {
        return state == CellState::Border;
    }
    return state == CellState::Border || state == CellState::ClaimedSlow || state == CellState::ClaimedFast;
}

bool Sparx::isPerimeterOrClaimed(CellState state) noexcept
{
    return state == CellState::Border || state == CellState::ClaimedSlow || state == CellState::ClaimedFast;
}

} // namespace qix
