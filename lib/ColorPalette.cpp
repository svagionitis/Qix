#include "ColorPalette.h"
#include <algorithm>
#include <array>
#include <cctype>
#include <string>

namespace qix {

namespace {

    constexpr PaletteTheme kClassicTheme {PaletteId::Classic, "Classic 1981", "Authentic 1981 Arcade",
        // Background & HUD
        PaletteColor {11, 15, 25, 255}, PaletteColor {5, 8, 15, 255}, PaletteColor {18, 24, 38, 255},
        PaletteColor {35, 45, 68, 255},
        // Playfield & Grid Cells
        PaletteColor {59, 130, 246, 255}, // Border (Vivid Blue)
        PaletteColor {14, 116, 144, 200}, // ClaimedSlow (Cyan/Teal)
        PaletteColor {180, 83, 9, 200}, // ClaimedFast (Amber/Orange)
        PaletteColor {255, 255, 255, 255}, // ActiveStix (White)
        // Entities
        PaletteColor {255, 255, 255, 255}, // Marker
        PaletteColor {245, 101, 101, 255}, // MarkerDiamond (Red)
        PaletteColor {255, 50, 220, 255}, // Sparx (Magenta)
        PaletteColor {0, 255, 255, 255}, // SuperSparx (Cyan)
        PaletteColor {255, 68, 68, 255}, // Fuse (Red)
        // HUD Text & Accents
        PaletteColor {160, 174, 192, 255}, // Label
        PaletteColor {246, 224, 94, 255}, // Value (Yellow)
        PaletteColor {99, 179, 237, 255}, // Accent (Sky blue)
        PaletteColor {30, 41, 59, 255}, // ProgressBg
        PaletteColor {59, 130, 246, 255}, // ProgressFill
        PaletteColor {72, 187, 120, 255}, // ProgressTarget (Green)
        RibbonColorMode::RainbowHsv};

    constexpr PaletteTheme kSynthwaveTheme {PaletteId::Synthwave, "Synthwave Neon", "Cyberpunk Midnight & Hot Pink",
        // Background & HUD
        PaletteColor {12, 8, 23, 255}, // Midnight violet
        PaletteColor {6, 4, 12, 255}, // Deep void
        PaletteColor {24, 15, 44, 255}, // Deep plum
        PaletteColor {55, 30, 95, 255}, // Violet border
        // Playfield & Grid Cells
        PaletteColor {0, 240, 255, 255}, // Border (Laser Cyan)
        PaletteColor {236, 72, 153, 200}, // ClaimedSlow (Neon Hot Pink)
        PaletteColor {126, 34, 206, 200}, // ClaimedFast (Neon Purple)
        PaletteColor {255, 255, 255, 255}, // ActiveStix
        // Entities
        PaletteColor {255, 230, 0, 255}, // Marker (Neon Yellow)
        PaletteColor {255, 0, 128, 255}, // MarkerDiamond (Magenta)
        PaletteColor {255, 0, 85, 255}, // Sparx (Laser Pink)
        PaletteColor {0, 255, 159, 255}, // SuperSparx (Neon Mint)
        PaletteColor {255, 107, 0, 255}, // Fuse (Cyber Orange)
        // HUD Text & Accents
        PaletteColor {180, 160, 220, 255}, // Label (Lavender)
        PaletteColor {255, 230, 0, 255}, // Value (Laser Yellow)
        PaletteColor {0, 240, 255, 255}, // Accent (Laser Cyan)
        PaletteColor {38, 20, 70, 255}, // ProgressBg
        PaletteColor {236, 72, 153, 255}, // ProgressFill (Pink)
        PaletteColor {0, 255, 159, 255}, // ProgressTarget (Mint)
        RibbonColorMode::NeonGradient};

    constexpr PaletteTheme kAmberTheme {PaletteId::Amber, "P3 Amber CRT", "Warm Amber Monochrome Phosphor",
        // Background & HUD
        PaletteColor {10, 6, 0, 255}, // Warm dark void
        PaletteColor {4, 2, 0, 255}, PaletteColor {26, 16, 0, 255}, // Dark amber HUD
        PaletteColor {60, 36, 0, 255}, // Amber HUD border
        // Playfield & Grid Cells
        PaletteColor {217, 119, 6, 255}, // Border (Warm Amber)
        PaletteColor {180, 83, 9, 200}, // ClaimedSlow (Rich Amber)
        PaletteColor {120, 53, 15, 200}, // ClaimedFast (Deep Amber)
        PaletteColor {254, 243, 199, 255}, // ActiveStix (Glowing Amber White)
        // Entities
        PaletteColor {245, 158, 11, 255}, // Marker (Pure Amber)
        PaletteColor {251, 191, 36, 255}, // MarkerDiamond
        PaletteColor {251, 191, 36, 255}, // Sparx
        PaletteColor {255, 251, 235, 255}, // SuperSparx (White Amber)
        PaletteColor {146, 64, 14, 255}, // Fuse
        // HUD Text & Accents
        PaletteColor {180, 130, 60, 255}, // Label
        PaletteColor {251, 191, 36, 255}, // Value
        PaletteColor {217, 119, 6, 255}, // Accent
        PaletteColor {40, 24, 0, 255}, // ProgressBg
        PaletteColor {217, 119, 6, 255}, // ProgressFill
        PaletteColor {251, 191, 36, 255}, // ProgressTarget
        RibbonColorMode::MonochromeAmber};

    constexpr PaletteTheme kGreenTheme {PaletteId::Green, "P1 Green CRT", "Vintage Green Monochrome Phosphor",
        // Background & HUD
        PaletteColor {0, 13, 4, 255}, // Vintage dark green void
        PaletteColor {0, 6, 2, 255}, PaletteColor {2, 30, 10, 255}, // Dark green HUD
        PaletteColor {10, 65, 25, 255}, // Green HUD border
        // Playfield & Grid Cells
        PaletteColor {34, 197, 94, 255}, // Border (Matrix Green)
        PaletteColor {21, 128, 61, 200}, // ClaimedSlow (Forest Jade)
        PaletteColor {20, 83, 45, 200}, // ClaimedFast (Dark Moss)
        PaletteColor {187, 247, 208, 255}, // ActiveStix (Mint White)
        // Entities
        PaletteColor {74, 222, 128, 255}, // Marker (Pure Phosphor Green)
        PaletteColor {134, 239, 172, 255}, // MarkerDiamond
        PaletteColor {163, 230, 53, 255}, // Sparx (Lime Green)
        PaletteColor {240, 253, 244, 255}, // SuperSparx (Mint White)
        PaletteColor {54, 83, 20, 255}, // Fuse
        // HUD Text & Accents
        PaletteColor {80, 160, 100, 255}, // Label
        PaletteColor {74, 222, 128, 255}, // Value
        PaletteColor {34, 197, 94, 255}, // Accent
        PaletteColor {5, 45, 18, 255}, // ProgressBg
        PaletteColor {34, 197, 94, 255}, // ProgressFill
        PaletteColor {134, 239, 172, 255}, // ProgressTarget
        RibbonColorMode::MonochromeGreen};

} // namespace

const PaletteTheme& ColorPalette::get(PaletteId id) noexcept
{
    switch (id) {
    case PaletteId::Classic:
        return kClassicTheme;
    case PaletteId::Synthwave:
        return kSynthwaveTheme;
    case PaletteId::Amber:
        return kAmberTheme;
    case PaletteId::Green:
        return kGreenTheme;
    default:
        return kClassicTheme;
    }
}

PaletteId ColorPalette::next(PaletteId current) noexcept
{
    switch (current) {
    case PaletteId::Classic:
        return PaletteId::Synthwave;
    case PaletteId::Synthwave:
        return PaletteId::Amber;
    case PaletteId::Amber:
        return PaletteId::Green;
    case PaletteId::Green:
        return PaletteId::Classic;
    default:
        return PaletteId::Classic;
    }
}

PaletteId ColorPalette::fromName(std::string_view name, PaletteId fallback) noexcept
{
    std::string lower;
    lower.reserve(name.size());
    for (char c : name) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (lower == "classic" || lower == "1981" || lower == "default") {
        return PaletteId::Classic;
    }
    if (lower == "synthwave" || lower == "cyberpunk" || lower == "neon") {
        return PaletteId::Synthwave;
    }
    if (lower == "amber" || lower == "p3" || lower == "orange") {
        return PaletteId::Amber;
    }
    if (lower == "green" || lower == "p1" || lower == "matrix") {
        return PaletteId::Green;
    }

    return fallback;
}

const char* ColorPalette::toString(PaletteId id) noexcept
{
    switch (id) {
    case PaletteId::Classic:
        return "Classic 1981";
    case PaletteId::Synthwave:
        return "Synthwave Neon";
    case PaletteId::Amber:
        return "P3 Amber CRT";
    case PaletteId::Green:
        return "P1 Green CRT";
    default:
        return "Classic 1981";
    }
}

} // namespace qix
