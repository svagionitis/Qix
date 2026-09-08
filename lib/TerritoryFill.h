#pragma once
#include "Playfield.h"
#include "Types.h"
#include <cstdint>
#include <vector>

namespace qix {

/// @brief Result statistics returned after territory evaluation.
struct FillResult {
    std::uint32_t claimedCellsCount {0};
    std::uint32_t totalClaimedSoFar {0};
    std::uint16_t claimedPercent {0};
    std::uint32_t pointsAwarded {0};
    std::uint32_t thresholdBonus {0};
    bool thresholdMet {false};
    bool splitOccurred {false};
};

/// @class TerritoryFill
/// @brief Connected-components flood-fill engine for territorial partitioning.
/// @details Isolates regions containing the Qix; marks uncontained regions as claimed.
class TerritoryFill {
public:
    /// @brief Construct fill system pre-allocating scratch buffers for given dimensions.
    /// @param[in] width Maximum playfield width.
    /// @param[in] height Maximum playfield height.
    TerritoryFill(std::int32_t width, std::int32_t height) noexcept;

    /// @brief Execute territory partitioning upon Stix loop closure.
    /// @param[in,out] field Active playfield to update with new borders and claimed cells.
    /// @param[in] trail Coordinates forming the completed Stix line.
    /// @param[in] qixPositions Coordinates of all active Qix entities.
    /// @param[in] mode Drawing speed used (DrawMode::Slow awards 2x points).
    /// @param[in] targetPercent Victory threshold (e.g. 75%).
    /// @param[in] multiplier Active score multiplier (1x up to 9x, default 1).
    /// @return FillResult detailing cells claimed, percentage, points, and whether a split occurred.
    FillResult execute(Playfield& field, const std::vector<Point>& trail, const std::vector<Point>& qixPositions,
        DrawMode mode, std::uint16_t targetPercent, std::uint8_t multiplier = 1) noexcept;

private:
    std::int32_t m_width {0};
    std::int32_t m_height {0};
    std::vector<std::uint8_t> m_visited {};
    std::vector<Point> m_queue {};

    [[nodiscard]] Point findSeed(const Playfield& field, Point startPos) const noexcept;
    void floodFromSeed(const Playfield& field, Point seed) noexcept;
};

} // namespace qix
