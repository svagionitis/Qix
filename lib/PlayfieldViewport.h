#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace qix {

/// @struct ViewportRect
/// @brief Generic floating-point rectangle for coordinate conversions.
struct ViewportRect {
    float x {0.0f};
    float y {0.0f};
    float width {0.0f};
    float height {0.0f};
};

/// @struct ViewportPixelRect
/// @brief Integer pixel rectangle for software rendering and texture blitting.
struct ViewportPixelRect {
    int x {0};
    int y {0};
    int width {0};
    int height {0};
};

/// @class PlayfieldViewport
/// @brief Coordinate transformation utility mapping playfield discrete grid cells to screen viewports.
/// @details Encapsulates zero-allocation geometry math, texture coordinate mapping, and center alignment
/// across all graphical frontend implementations (Raylib, SDL2, Qt6, TUI).
class PlayfieldViewport {
public:
    /// @brief Construct a viewport with field boundaries and grid dimension.
    /// @param[in] fieldX Left pixel offset of the playfield.
    /// @param[in] fieldY Top pixel offset of the playfield.
    /// @param[in] fieldW Total width of the playfield in pixels.
    /// @param[in] fieldH Total height of the playfield in pixels.
    /// @param[in] gridW Width in discrete cells (default 80).
    /// @param[in] gridH Height in discrete cells (default 60).
    constexpr PlayfieldViewport(float fieldX, float fieldY, float fieldW, float fieldH, std::int32_t gridW = 80,
        std::int32_t gridH = 60) noexcept
        : m_fieldX {fieldX}
        , m_fieldY {fieldY}
        , m_fieldW {fieldW}
        , m_fieldH {fieldH}
        , m_gridW {gridW > 0 ? gridW : 1}
        , m_gridH {gridH > 0 ? gridH : 1}
        , m_cellW {fieldW / static_cast<float>(gridW > 0 ? gridW : 1)}
        , m_cellH {fieldH / static_cast<float>(gridH > 0 ? gridH : 1)}
    {
    }

    /// @brief Calculate the screen bounding box for a grid cell.
    /// @param[in] gx Cell horizontal grid index.
    /// @param[in] gy Cell vertical grid index.
    /// @param[in] overlap Extra pixel overlap for seamless rendering (default 0.5f).
    /// @return Floating-point ViewportRect in screen coordinates.
    [[nodiscard]] constexpr ViewportRect cellToScreen(
        std::int32_t gx, std::int32_t gy, float overlap = 0.5f) const noexcept
    {
        return ViewportRect {m_fieldX + static_cast<float>(gx) * m_cellW, m_fieldY + static_cast<float>(gy) * m_cellH,
            m_cellW + overlap, m_cellH + overlap};
    }

    /// @brief Calculate the integer pixel bounding box for a grid cell.
    /// @param[in] gx Cell horizontal grid index.
    /// @param[in] gy Cell vertical grid index.
    /// @return Integer ViewportPixelRect in screen coordinates.
    [[nodiscard]] constexpr ViewportPixelRect cellToScreenPixel(std::int32_t gx, std::int32_t gy) const noexcept
    {
        const double fx {static_cast<double>(m_fieldX)};
        const double fy {static_cast<double>(m_fieldY)};
        const double cw {static_cast<double>(m_fieldW) / static_cast<double>(m_gridW)};
        const double ch {static_cast<double>(m_fieldH) / static_cast<double>(m_gridH)};
        return ViewportPixelRect {static_cast<int>(fx + static_cast<double>(gx) * cw),
            static_cast<int>(fy + static_cast<double>(gy) * ch), static_cast<int>(cw + 0.99),
            static_cast<int>(ch + 0.99)};
    }

    /// @brief Calculate texture sub-rectangle for mapping background art to a grid cell.
    /// @param[in] gx Cell horizontal grid index.
    /// @param[in] gy Cell vertical grid index.
    /// @param[in] texW Texture width in pixels.
    /// @param[in] texH Texture height in pixels.
    /// @return Integer ViewportPixelRect matching texture source rectangle.
    [[nodiscard]] ViewportPixelRect cellToTextureSrc(
        std::int32_t gx, std::int32_t gy, int texW, int texH) const noexcept
    {
        const double tw = static_cast<double>(texW);
        const double th = static_cast<double>(texH);
        return ViewportPixelRect {static_cast<int>((static_cast<double>(gx) / m_gridW) * tw),
            static_cast<int>((static_cast<double>(gy) / m_gridH) * th),
            std::max(1, static_cast<int>(std::ceil((1.0 / m_gridW) * tw))),
            std::max(1, static_cast<int>(std::ceil((1.0 / m_gridH) * th)))};
    }

    /// @brief Calculate centered horizontal coordinate for a text string or box.
    /// @param[in] elementWidth Width of element to center.
    /// @param[in] containerWidth Width of container (e.g. screen width).
    /// @param[in] minMargin Minimum margin from the left edge (default 10).
    /// @return Left X coordinate centered inside container.
    [[nodiscard]] static constexpr int centerX(int elementWidth, int containerWidth, int minMargin = 10) noexcept
    {
        const int pos = (containerWidth - elementWidth) / 2;
        return pos < minMargin ? minMargin : pos;
    }

    /// @brief Determine if a periodic blink state is active (on).
    /// @param[in] ticksMs Monotonic timestamp in milliseconds.
    /// @param[in] halfPeriodMs Half-period duration in milliseconds (default 350).
    /// @return True during the "visible" phase of the blink cycle.
    [[nodiscard]] static constexpr bool isBlinkOn(std::uint32_t ticksMs, std::uint32_t halfPeriodMs = 350U) noexcept
    {
        return ((ticksMs / (halfPeriodMs > 0U ? halfPeriodMs : 1U)) % 2U) == 0U;
    }

    [[nodiscard]] constexpr float cellWidth() const noexcept
    {
        return m_cellW;
    }
    [[nodiscard]] constexpr float cellHeight() const noexcept
    {
        return m_cellH;
    }
    [[nodiscard]] constexpr std::int32_t gridWidth() const noexcept
    {
        return m_gridW;
    }
    [[nodiscard]] constexpr std::int32_t gridHeight() const noexcept
    {
        return m_gridH;
    }

private:
    float m_fieldX {0.0f};
    float m_fieldY {0.0f};
    float m_fieldW {0.0f};
    float m_fieldH {0.0f};
    std::int32_t m_gridW {80};
    std::int32_t m_gridH {60};
    float m_cellW {0.0f};
    float m_cellH {0.0f};
};

} // namespace qix
