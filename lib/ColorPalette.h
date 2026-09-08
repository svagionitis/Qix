#pragma once
#include <cstdint>
#include <string_view>

namespace qix {

/// @enum PaletteId
/// @brief Identifier for curated arcade visual color palettes.
enum class PaletteId : std::uint8_t { Classic = 0, Synthwave = 1, Amber = 2, Green = 3 };

/// @struct PaletteColor
/// @brief 32-bit RGBA color representation.
struct PaletteColor {
    std::uint8_t r {0};
    std::uint8_t g {0};
    std::uint8_t b {0};
    std::uint8_t a {255};

    constexpr bool operator==(const PaletteColor& other) const noexcept
    {
        return r == other.r && g == other.g && b == other.b && a == other.a;
    }

    constexpr bool operator!=(const PaletteColor& other) const noexcept
    {
        return !(*this == other);
    }
};

/// @enum RibbonColorMode
/// @brief Vector ribbon styling modes for the Qix entity.
enum class RibbonColorMode : std::uint8_t {
    RainbowHsv = 0,
    NeonGradient = 1,
    MonochromeAmber = 2,
    MonochromeGreen = 3
};

/// @struct PaletteTheme
/// @brief Comprehensive theme token collection for playfield, entities, HUD, and CRT filters.
struct PaletteTheme {
    PaletteId id {PaletteId::Classic};
    const char* name {"Classic 1981"};
    const char* description {"Authentic 1981 Arcade"};

    // Background & HUD
    PaletteColor background {11, 15, 25, 255};
    PaletteColor crtBackdrop {5, 8, 15, 255};
    PaletteColor hudBg {18, 24, 38, 255};
    PaletteColor hudBorder {35, 45, 68, 255};

    // Playfield & Grid Cells
    PaletteColor playfieldBorder {59, 130, 246, 255};
    PaletteColor claimedSlow {14, 116, 144, 200};
    PaletteColor claimedFast {180, 83, 9, 200};
    PaletteColor activeStix {255, 255, 255, 255};

    // Entities
    PaletteColor marker {255, 255, 255, 255};
    PaletteColor markerDiamond {245, 101, 101, 255};
    PaletteColor sparx {255, 50, 220, 255};
    PaletteColor superSparx {0, 255, 255, 255};
    PaletteColor fuse {255, 68, 68, 255};

    // HUD Text & Accents
    PaletteColor textLabel {160, 174, 192, 255};
    PaletteColor textValue {246, 224, 94, 255};
    PaletteColor textAccent {99, 179, 237, 255};
    PaletteColor progressBarBg {30, 41, 59, 255};
    PaletteColor progressBarFill {59, 130, 246, 255};
    PaletteColor progressBarTarget {72, 187, 120, 255};

    // Ribbon style
    RibbonColorMode ribbonMode {RibbonColorMode::RainbowHsv};
};

/// @class ColorPalette
/// @brief Registry and helper functions for arcade color palettes.
class ColorPalette {
public:
    /// @brief Retrieve palette theme definition by ID.
    /// @param[in] id Palette identifier.
    /// @return Immutable reference to PaletteTheme.
    [[nodiscard]] static const PaletteTheme& get(PaletteId id) noexcept;

    /// @brief Get next palette in cycle order (Classic -> Synthwave -> Amber -> Green -> Classic).
    /// @param[in] current Current palette identifier.
    /// @return Next palette identifier.
    [[nodiscard]] static PaletteId next(PaletteId current) noexcept;

    /// @brief Parse palette identifier from string name.
    /// @param[in] name Palette name string (e.g. "classic", "synthwave", "amber", "green").
    /// @param[in] fallback Fallback palette if name is unrecognized (default: Classic).
    /// @return Parsed PaletteId.
    [[nodiscard]] static PaletteId fromName(std::string_view name, PaletteId fallback = PaletteId::Classic) noexcept;

    /// @brief Convert palette identifier to string representation.
    /// @param[in] id Palette identifier.
    /// @return Constant character pointer to palette name.
    [[nodiscard]] static const char* toString(PaletteId id) noexcept;

    /// @brief Compute dynamic ribbon segment color based on palette theme, animation cycle, and segment index.
    /// @param[in] theme The active palette theme.
    /// @param[in] cycle Monotonically advancing animation cycle counter.
    /// @param[in] segIdx Index of the ribbon segment (0 is leading segment).
    /// @param[in] totalSegs Total number of segments in the ribbon.
    /// @return 32-bit RGBA PaletteColor.
    [[nodiscard]] static PaletteColor computeRibbonColor(
        const PaletteTheme& theme, std::uint32_t cycle, std::size_t segIdx, std::size_t totalSegs) noexcept;
};

} // namespace qix
