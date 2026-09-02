#include "TerritoryFill.h"
#include <algorithm>

namespace qix {

TerritoryFill::TerritoryFill(std::int32_t width, std::int32_t height) noexcept
    : m_width {width}
    , m_height {height}
{
    const auto total = static_cast<std::size_t>(m_width) * static_cast<std::size_t>(m_height);
    m_visited.resize(total, 0);
    m_queue.reserve(total);
}

FillResult TerritoryFill::execute(Playfield& field, const std::vector<Point>& trail,
    const std::vector<Point>& qixPositions, DrawMode mode, std::uint16_t targetPercent) noexcept
{
    // Convert all points along the completed Stix line into permanent borders
    for (const auto& pt : trail) {
        field.setCell(pt.x, pt.y, CellState::Border);
    }

    // Reset pre-allocated visited buffer
    std::fill(m_visited.begin(), m_visited.end(), 0);

    // Flood-fill all empty regions reachable by any Qix entity
    for (const auto& qpos : qixPositions) {
        floodFromQix(field, qpos);
    }

    // Any empty cell unvisited by the Qix flood-fill is an enclosed region to be claimed
    const auto fillState = (mode == DrawMode::Slow) ? CellState::ClaimedSlow : CellState::ClaimedFast;
    std::uint32_t freshlyClaimed {0};

    for (std::int32_t y {1}; y < field.getHeight() - 1; ++y) {
        for (std::int32_t x {1}; x < field.getWidth() - 1; ++x) {
            const auto idx = static_cast<std::size_t>(y) * static_cast<std::size_t>(field.getWidth())
                + static_cast<std::size_t>(x);
            if (field.getCell(x, y) == CellState::Empty && m_visited[idx] == 0) {
                field.setCell(x, y, fillState);
                ++freshlyClaimed;
            }
        }
    }

    field.updateClaimedCount();

    FillResult result {};
    result.claimedCellsCount = freshlyClaimed;
    result.totalClaimedSoFar = field.getClaimedCount();

    const auto totalPlayable = field.getInteriorCount();
    if (totalPlayable > 0) {
        result.claimedPercent
            = static_cast<std::uint16_t>((static_cast<std::uint64_t>(result.totalClaimedSoFar) * 100) / totalPlayable);
    }

    // Slow draw awards double points (200 pts per cell vs 100 pts)
    const std::uint32_t ptsPerCell = (mode == DrawMode::Slow) ? 200U : 100U;
    result.pointsAwarded = freshlyClaimed * ptsPerCell;
    result.thresholdMet = (result.claimedPercent >= targetPercent);

    return result;
}

void TerritoryFill::floodFromQix(const Playfield& field, Point startPos) noexcept
{
    m_queue.clear();

    // Ensure start coordinate is an empty cell; if not, check immediate neighbors
    Point seed = startPos;
    if (field.getCell(seed.x, seed.y) != CellState::Empty) {
        bool foundSeed = false;
        for (std::int32_t dy {-1}; dy <= 1 && !foundSeed; ++dy) {
            for (std::int32_t dx {-1}; dx <= 1 && !foundSeed; ++dx) {
                const auto nx = startPos.x + dx;
                const auto ny = startPos.y + dy;
                if (field.isInBounds(nx, ny) && field.getCell(nx, ny) == CellState::Empty) {
                    seed = Point {nx, ny};
                    foundSeed = true;
                }
            }
        }
        if (!foundSeed) {
            return;
        }
    }

    const auto width = field.getWidth();
    const auto seedIdx
        = static_cast<std::size_t>(seed.y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(seed.x);
    m_visited[seedIdx] = 1;
    m_queue.push_back(seed);

    std::size_t headIndex {0};

    // Breadth-first traversal without dynamic allocations
    while (headIndex < m_queue.size()) {
        const auto curr = m_queue[headIndex];
        ++headIndex;

        const Point neighbors[4] {
            {curr.x, curr.y - 1}, {curr.x, curr.y + 1}, {curr.x - 1, curr.y}, {curr.x + 1, curr.y}};

        for (const auto& nb : neighbors) {
            if (!field.isInBounds(nb.x, nb.y)) {
                continue;
            }

            const auto idx
                = static_cast<std::size_t>(nb.y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(nb.x);
            if (m_visited[idx] == 0 && field.getCell(nb.x, nb.y) == CellState::Empty) {
                m_visited[idx] = 1;
                m_queue.push_back(nb);
            }
        }
    }
}

} // namespace qix
