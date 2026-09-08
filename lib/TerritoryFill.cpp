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

bool TerritoryFill::detectSpiralTrail(const std::vector<Point>& trail) noexcept
{
    if (trail.size() < 5) {
        return false;
    }

    struct DirVec {
        std::int32_t dx {0};
        std::int32_t dy {0};
    };

    std::vector<DirVec> turnDirs {};
    turnDirs.reserve(trail.size());

    DirVec currentDir {0, 0};
    for (std::size_t i {1}; i < trail.size(); ++i) {
        const auto dx = trail[i].x - trail[i - 1].x;
        const auto dy = trail[i].y - trail[i - 1].y;
        if (dx == 0 && dy == 0) {
            continue;
        }

        const auto stepX = (dx > 0) ? 1 : ((dx < 0) ? -1 : 0);
        const auto stepY = (dy > 0) ? 1 : ((dy < 0) ? -1 : 0);

        if (currentDir.dx == 0 && currentDir.dy == 0) {
            currentDir = DirVec {stepX, stepY};
        } else if (stepX != currentDir.dx || stepY != currentDir.dy) {
            turnDirs.push_back(currentDir);
            currentDir = DirVec {stepX, stepY};
        }
    }
    if (currentDir.dx != 0 || currentDir.dy != 0) {
        turnDirs.push_back(currentDir);
    }

    if (turnDirs.size() < 4) {
        return false;
    }

    std::int32_t consecutiveSameSign {0};
    std::int32_t maxConsecutiveSameSign {0};
    std::int32_t lastSign {0};
    std::size_t totalTurns {0};

    for (std::size_t i {1}; i < turnDirs.size(); ++i) {
        const auto& d1 = turnDirs[i - 1];
        const auto& d2 = turnDirs[i];
        const auto cross = d1.dx * d2.dy - d1.dy * d2.dx;
        if (cross == 0) {
            continue;
        }

        ++totalTurns;
        const auto sign = (cross > 0) ? 1 : -1;
        if (sign == lastSign) {
            ++consecutiveSameSign;
        } else {
            consecutiveSameSign = 1;
            lastSign = sign;
        }
        if (consecutiveSameSign > maxConsecutiveSameSign) {
            maxConsecutiveSameSign = consecutiveSameSign;
        }
    }

    return (maxConsecutiveSameSign >= 3) || (totalTurns >= 4 && maxConsecutiveSameSign >= 2);
}

FillResult TerritoryFill::execute(Playfield& field, const std::vector<Point>& trail,
    const std::vector<Point>& qixPositions, DrawMode mode, std::uint16_t targetPercent,
    std::uint8_t multiplier) noexcept
{
    // Convert all points along the completed Stix line into permanent borders
    for (const auto& pt : trail) {
        field.setCell(pt.x, pt.y, CellState::Border);
    }

    // Reset pre-allocated visited buffer
    std::fill(m_visited.begin(), m_visited.end(), std::uint8_t {0});

    // Locate seeds for all Qix positions
    std::vector<Point> validSeeds {};
    validSeeds.reserve(qixPositions.size());
    for (const auto& qpos : qixPositions) {
        const auto seed = findSeed(field, qpos);
        if (seed.x != -1 && seed.y != -1) {
            validSeeds.push_back(seed);
        }
    }

    bool splitOccurred {false};

    if (!validSeeds.empty()) {
        // Flood from first Qix seed
        floodFromSeed(field, validSeeds[0]);

        // If multiple Qixes are present, check whether any subsequent Qix was unreachable
        if (validSeeds.size() >= 2) {
            const auto width = field.getWidth();
            for (std::size_t i {1}; i < validSeeds.size(); ++i) {
                const auto idx = static_cast<std::size_t>(validSeeds[i].y) * static_cast<std::size_t>(width)
                    + static_cast<std::size_t>(validSeeds[i].x);
                if (m_visited[idx] == 0) {
                    splitOccurred = true;
                    // Flood fill remaining Qix region so its territory is also preserved
                    floodFromSeed(field, validSeeds[i]);
                }
            }
        }
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

    // Count remaining empty cells accessible to the Qix
    std::uint32_t qixRemainingCells {0};
    for (std::int32_t y {1}; y < field.getHeight() - 1; ++y) {
        for (std::int32_t x {1}; x < field.getWidth() - 1; ++x) {
            const auto idx = static_cast<std::size_t>(y) * static_cast<std::size_t>(field.getWidth())
                + static_cast<std::size_t>(x);
            if (field.getCell(x, y) == CellState::Empty && m_visited[idx] != 0) {
                ++qixRemainingCells;
            }
        }
    }

    if (totalPlayable > 0) {
        result.qixRemainingPercent
            = static_cast<std::uint16_t>((static_cast<std::uint64_t>(qixRemainingCells) * 100) / totalPlayable);
    }

    const auto safeMultiplier = std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(multiplier));
    const std::uint32_t basePtsPerCell = (mode == DrawMode::Slow) ? 200U : 100U;
    result.pointsAwarded = freshlyClaimed * basePtsPerCell * safeMultiplier;
    result.splitOccurred = splitOccurred;
    result.thresholdMet = splitOccurred || (result.claimedPercent >= targetPercent);

    if (result.claimedPercent > targetPercent) {
        const auto overshoot = static_cast<std::uint32_t>(result.claimedPercent - targetPercent);
        result.thresholdBonus = overshoot * 1000U * safeMultiplier;
        result.pointsAwarded += result.thresholdBonus;
    }

    // Detect The Qix Trap and Spiral Bonus (remaining Qix area <= 10% or <= 5%)
    if (freshlyClaimed > 0 && qixRemainingCells > 0 && result.qixRemainingPercent <= 10) {
        result.qixTrapped = true;
        result.spiralBonus = detectSpiralTrail(trail);

        std::uint32_t baseTrapBonus = (result.qixRemainingPercent <= 5) ? 50000U : 25000U;
        if (result.spiralBonus) {
            baseTrapBonus += 25000U;
        }

        // Slow draw awards 2x bonus points
        if (mode == DrawMode::Slow) {
            baseTrapBonus *= 2U;
        }

        result.trapBonus = baseTrapBonus * safeMultiplier;
        result.pointsAwarded += result.trapBonus;
        result.thresholdMet = true; // Trapping the Qix into <= 10% always achieves victory
    }

    return result;
}

Point TerritoryFill::findSeed(const Playfield& field, Point startPos) const noexcept
{
    if (field.isInBounds(startPos.x, startPos.y) && field.getCell(startPos.x, startPos.y) == CellState::Empty) {
        return startPos;
    }

    for (std::int32_t dy {-1}; dy <= 1; ++dy) {
        for (std::int32_t dx {-1}; dx <= 1; ++dx) {
            const auto nx = startPos.x + dx;
            const auto ny = startPos.y + dy;
            if (field.isInBounds(nx, ny) && field.getCell(nx, ny) == CellState::Empty) {
                return Point {nx, ny};
            }
        }
    }
    return Point {-1, -1};
}

void TerritoryFill::floodFromSeed(const Playfield& field, Point seed) noexcept
{
    if (!field.isInBounds(seed.x, seed.y) || field.getCell(seed.x, seed.y) != CellState::Empty) {
        return;
    }

    const auto width = field.getWidth();
    const auto seedIdx
        = static_cast<std::size_t>(seed.y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(seed.x);
    if (m_visited[seedIdx] != 0) {
        return;
    }

    m_queue.clear();
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
