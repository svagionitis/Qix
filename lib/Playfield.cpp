#include "Playfield.h"

namespace qix {

Playfield::Playfield(std::int32_t width, std::int32_t height) noexcept
    : m_width {width}
    , m_height {height}
{
    if (m_width < 4) {
        m_width = 4;
    }
    if (m_height < 4) {
        m_height = 4;
    }

    const auto totalCells = static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height);
    m_cells.resize(totalCells, CellState::Empty);

    // Compute playable area excluding the outer 1-cell perimeter border
    m_interiorCount = static_cast<std::uint32_t>(m_width - 2) * static_cast<std::uint32_t>(m_height - 2);
    initBorders();
}

void Playfield::initBorders() noexcept
{
    // Fill all cells with Empty initially
    std::fill(m_cells.begin(), m_cells.end(), CellState::Empty);

    // Mark top and bottom boundary rows
    for (std::int32_t x {0}; x < m_width; ++x) {
        m_cells[toIndex(x, 0)] = CellState::Border;
        m_cells[toIndex(x, m_height - 1)] = CellState::Border;
    }

    // Mark left and right boundary columns
    for (std::int32_t y {0}; y < m_height; ++y) {
        m_cells[toIndex(0, y)] = CellState::Border;
        m_cells[toIndex(m_width - 1, y)] = CellState::Border;
    }

    m_claimedCount = 0;
}

CellState Playfield::getCell(std::int32_t x, std::int32_t y) const noexcept
{
    if (!isInBounds(x, y)) {
        return CellState::Border;
    }

    return m_cells[toIndex(x, y)];
}

void Playfield::setCell(std::int32_t x, std::int32_t y, CellState state) noexcept
{
    if (!isInBounds(x, y)) {
        return;
    }

    m_cells[toIndex(x, y)] = state;
}

bool Playfield::isInBounds(std::int32_t x, std::int32_t y) const noexcept
{
    if (x < 0 || x >= m_width) {
        return false;
    }
    if (y < 0 || y >= m_height) {
        return false;
    }

    return true;
}

std::int32_t Playfield::getWidth() const noexcept
{
    return m_width;
}

std::int32_t Playfield::getHeight() const noexcept
{
    return m_height;
}

std::uint32_t Playfield::getInteriorCount() const noexcept
{
    return m_interiorCount;
}

std::uint32_t Playfield::getClaimedCount() const noexcept
{
    return m_claimedCount;
}

void Playfield::updateClaimedCount() noexcept
{
    std::uint32_t count {0};

    for (std::int32_t y {1}; y < m_height - 1; ++y) {
        for (std::int32_t x {1}; x < m_width - 1; ++x) {
            const auto state = m_cells[toIndex(x, y)];
            if (state == CellState::ClaimedSlow || state == CellState::ClaimedFast) {
                ++count;
            }
        }
    }

    m_claimedCount = count;
}

std::size_t Playfield::toIndex(std::int32_t x, std::int32_t y) const noexcept
{
    return static_cast<std::size_t>(y) * static_cast<std::size_t>(m_width) + static_cast<std::size_t>(x);
}

} // namespace qix
