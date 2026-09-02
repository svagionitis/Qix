#ifndef QIX_LIB_PLAYFIELD_H
#define QIX_LIB_PLAYFIELD_H

#include "Types.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace qix {

/// @class Playfield
/// @brief Discrete 2D spatial grid storing boundary, claimed, and empty cell states.
/// @details Invariant: dimensions are fixed upon creation; indices are validated before access.
class Playfield {
public:
    /// @brief Construct a playfield with specified dimensions.
    /// @param[in] width Horizontal width in cell units.
    /// @param[in] height Vertical height in cell units.
    Playfield(std::int32_t width, std::int32_t height) noexcept;

    /// @brief Initialize outer boundary and mark inside as empty.
    void initBorders() noexcept;

    /// @brief Retrieve cell state at given coordinate.
    /// @param[in] x Coordinate along X-axis.
    /// @param[in] y Coordinate along Y-axis.
    /// @return CellState at (x, y) or CellState::Border if out of bounds.
    [[nodiscard]] CellState getCell(std::int32_t x, std::int32_t y) const noexcept;

    /// @brief Set cell state at given coordinate.
    /// @param[in] x Coordinate along X-axis.
    /// @param[in] y Coordinate along Y-axis.
    /// @param[in] state New state for the cell.
    void setCell(std::int32_t x, std::int32_t y, CellState state) noexcept;

    /// @brief Validate whether coordinate lies within bounds.
    /// @param[in] x Coordinate along X-axis.
    /// @param[in] y Coordinate along Y-axis.
    /// @return True if within bounds, false otherwise.
    [[nodiscard]] bool isInBounds(std::int32_t x, std::int32_t y) const noexcept;

    /// @brief Query playfield width.
    /// @return Width in cells.
    [[nodiscard]] std::int32_t getWidth() const noexcept;

    /// @brief Query playfield height.
    /// @return Height in cells.
    [[nodiscard]] std::int32_t getHeight() const noexcept;

    /// @brief Total count of interior cells eligible for capture.
    /// @return Cell count.
    [[nodiscard]] std::uint32_t getInteriorCount() const noexcept;

    /// @brief Total count of currently claimed cells.
    /// @return Claimed cell count.
    [[nodiscard]] std::uint32_t getClaimedCount() const noexcept;

    /// @brief Recalculate count of claimed cells.
    void updateClaimedCount() noexcept;

private:
    std::int32_t m_width {0};
    std::int32_t m_height {0};
    std::uint32_t m_interiorCount {0};
    std::uint32_t m_claimedCount {0};
    std::vector<CellState> m_cells {};

    [[nodiscard]] std::size_t toIndex(std::int32_t x, std::int32_t y) const noexcept;
};

} // namespace qix

#endif // QIX_LIB_PLAYFIELD_H
