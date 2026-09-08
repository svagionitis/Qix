#pragma once
#include "ColorPalette.h"
#include <cstdint>
#include <vector>

namespace qix {

/// @enum ArtScene
/// @brief Identifiers for procedural background artwork scenes.
enum class ArtScene : std::uint8_t {
    CyberpunkSkyline = 0,
    SynthwaveSunset = 1,
    CosmicNebula = 2,
    LaserMandala = 3,
    Count = 4
};

/// @class BackgroundArt
/// @brief High-performance, zero-asset procedural arcade artwork synthesis engine.
/// @details Generates deterministic 24-bit RGB/RGBA retro art scenes at arbitrary resolution
/// or via sub-microsecond analytical normalized pixel sampling.
class BackgroundArt {
public:
    /// @brief Total number of distinct artwork scenes available.
    static constexpr std::size_t SceneCount = static_cast<std::size_t>(ArtScene::Count);

    /// @brief Sample an RGB color from a scene at normalized coordinates (u, v) in [0.0, 1.0].
    /// @param[in] scene The artwork scene to render.
    /// @param[in] u Horizontal normalized coordinate [0.0, 1.0] (left to right).
    /// @param[in] v Vertical normalized coordinate [0.0, 1.0] (top to bottom).
    /// @return 32-bit RGBA PaletteColor (alpha is always 255).
    [[nodiscard]] static PaletteColor samplePixel(ArtScene scene, float u, float v) noexcept;

    /// @brief Pre-generate a full 32-bit RGBA pixel buffer for an artwork scene.
    /// @param[in] scene The artwork scene to render.
    /// @param[in] width Target buffer width in pixels.
    /// @param[in] height Target buffer height in pixels.
    /// @param[out] outBuffer Output buffer vector formatted as [R, G, B, A, R, G, B, A, ...].
    static void generateRgbaBuffer(
        ArtScene scene, int width, int height, std::vector<std::uint8_t>& outBuffer) noexcept;

    /// @brief Apply cool-cyan fast-draw tint to an artwork color.
    /// @param[in] color The raw artwork PaletteColor.
    /// @return Muted, cool-cyan tinted PaletteColor.
    [[nodiscard]] static constexpr PaletteColor tintForFastDraw(const PaletteColor& color) noexcept
    {
        const int g = (static_cast<int>(color.g) * 2 / 3) + 30;
        const int b = (static_cast<int>(color.b) * 4 / 5) + 60;
        return PaletteColor {
            static_cast<std::uint8_t>(color.r / 3),
            static_cast<std::uint8_t>(g > 255 ? 255 : g),
            static_cast<std::uint8_t>(b > 255 ? 255 : b),
            color.a
        };
    }

    /// @brief Pre-generate both standard and muted fast-draw RGBA buffers for an artwork scene.
    /// @param[in] scene The artwork scene to render.
    /// @param[in] width Target buffer width in pixels.
    /// @param[in] height Target buffer height in pixels.
    /// @param[out] standardBuffer Output standard 100% saturation buffer.
    /// @param[out] mutedBuffer Output muted/cyan-tinted buffer for Fast Draw.
    static void generateDualRgbaBuffers(
        ArtScene scene, int width, int height,
        std::vector<std::uint8_t>& standardBuffer,
        std::vector<std::uint8_t>& mutedBuffer) noexcept;

    /// @brief Retrieve the human-readable display name of an artwork scene.
    /// @param[in] scene The artwork scene enum.
    /// @return String title (e.g. "Cyberpunk Skyline").
    [[nodiscard]] static const char* getSceneName(ArtScene scene) noexcept;

    /// @brief Retrieve the default artwork scene for a given game level.
    /// @param[in] level Current game level (1-indexed).
    /// @return Matching ArtScene enum cycling across levels.
    [[nodiscard]] static ArtScene getSceneForLevel(std::uint8_t level) noexcept;

    /// @brief Convert an integer index to an ArtScene enum with bounds wrapping.
    /// @param[in] index Scene integer index.
    /// @return Valid ArtScene.
    [[nodiscard]] static ArtScene fromIndex(int index) noexcept;

private:
    [[nodiscard]] static PaletteColor sampleCyberpunk(float u, float v) noexcept;
    [[nodiscard]] static PaletteColor sampleSynthwave(float u, float v) noexcept;
    [[nodiscard]] static PaletteColor sampleCosmic(float u, float v) noexcept;
    [[nodiscard]] static PaletteColor sampleMandala(float u, float v) noexcept;
};

} // namespace qix
