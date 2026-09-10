#include "BackgroundArt.h"
#include <algorithm>
#include <cmath>

namespace qix {

namespace {

    [[nodiscard]] inline float clamp01(float val) noexcept
    {
        return std::clamp(val, 0.0f, 1.0f);
    }

    [[nodiscard]] inline std::uint8_t toByte(float v) noexcept
    {
        const auto clamped = std::clamp(v, 0.0f, 255.0f);
        return static_cast<std::uint8_t>(clamped + 0.5f);
    }

    [[nodiscard]] inline PaletteColor lerpColor(const PaletteColor& c1, const PaletteColor& c2, float t) noexcept
    {
        const float factor = clamp01(t);
        return PaletteColor {
            toByte(static_cast<float>(c1.r) + (static_cast<float>(c2.r) - static_cast<float>(c1.r)) * factor),
            toByte(static_cast<float>(c1.g) + (static_cast<float>(c2.g) - static_cast<float>(c1.g)) * factor),
            toByte(static_cast<float>(c1.b) + (static_cast<float>(c2.b) - static_cast<float>(c1.b)) * factor), 255};
    }

    [[nodiscard]] inline PaletteColor addGlow(
        const PaletteColor& base, float r, float g, float b, float intensity) noexcept
    {
        const float scale = std::max(0.0f, intensity);
        return PaletteColor {toByte(static_cast<float>(base.r) + r * scale),
            toByte(static_cast<float>(base.g) + g * scale), toByte(static_cast<float>(base.b) + b * scale), 255};
    }

    // Deterministic 1D/2D pseudo-random hash
    [[nodiscard]] inline float hash11(float p) noexcept
    {
        float pInt = 0.0f;
        float pFract = std::modf(std::sin(p * 127.1f) * 43758.5453123f, &pInt);
        if (pFract < 0.0f) {
            pFract += 1.0f;
        }
        return pFract;
    }

    [[nodiscard]] inline float hash21(float x, float y) noexcept
    {
        float pInt = 0.0f;
        float pFract = std::modf(std::sin(x * 12.9898f + y * 78.233f) * 43758.5453f, &pInt);
        if (pFract < 0.0f) {
            pFract += 1.0f;
        }
        return pFract;
    }

} // namespace

PaletteColor BackgroundArt::samplePixel(ArtScene scene, float u, float v) noexcept
{
    const float cu = clamp01(u);
    const float cv = clamp01(v);

    switch (scene) {
    case ArtScene::CyberpunkSkyline:
        return sampleCyberpunk(cu, cv);
    case ArtScene::SynthwaveSunset:
        return sampleSynthwave(cu, cv);
    case ArtScene::CosmicNebula:
        return sampleCosmic(cu, cv);
    case ArtScene::LaserMandala:
        return sampleMandala(cu, cv);
    default:
        return sampleCyberpunk(cu, cv);
    }
}

void BackgroundArt::generateRgbaBuffer(
    ArtScene scene, int width, int height, std::vector<std::uint8_t>& outBuffer) noexcept
{
    if (width <= 0 || height <= 0) {
        outBuffer.clear();
        return;
    }

    const auto totalBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    outBuffer.resize(totalBytes);

    const float invW = 1.0f / static_cast<float>(width > 1 ? (width - 1) : 1);
    const float invH = 1.0f / static_cast<float>(height > 1 ? (height - 1) : 1);

    std::size_t offset {0};
    for (int y {0}; y < height; ++y) {
        const float v = static_cast<float>(y) * invH;
        for (int x {0}; x < width; ++x) {
            const float u = static_cast<float>(x) * invW;
            const auto color = samplePixel(scene, u, v);
            outBuffer[offset] = color.r;
            outBuffer[offset + 1] = color.g;
            outBuffer[offset + 2] = color.b;
            outBuffer[offset + 3] = 255;
            offset += 4U;
        }
    }
}

void BackgroundArt::generateDualRgbaBuffers(ArtScene scene, int width, int height,
    std::vector<std::uint8_t>& standardBuffer, std::vector<std::uint8_t>& mutedBuffer) noexcept
{
    if (width <= 0 || height <= 0) {
        standardBuffer.clear();
        mutedBuffer.clear();
        return;
    }

    const auto totalBytes = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    standardBuffer.resize(totalBytes);
    mutedBuffer.resize(totalBytes);

    const float invW = 1.0f / static_cast<float>(width > 1 ? (width - 1) : 1);
    const float invH = 1.0f / static_cast<float>(height > 1 ? (height - 1) : 1);

    std::size_t offset {0};
    for (int y {0}; y < height; ++y) {
        const float v = static_cast<float>(y) * invH;
        for (int x {0}; x < width; ++x) {
            const float u = static_cast<float>(x) * invW;
            const auto color = samplePixel(scene, u, v);
            const auto muted = tintForFastDraw(color);

            standardBuffer[offset] = color.r;
            standardBuffer[offset + 1] = color.g;
            standardBuffer[offset + 2] = color.b;
            standardBuffer[offset + 3] = 255;

            mutedBuffer[offset] = muted.r;
            mutedBuffer[offset + 1] = muted.g;
            mutedBuffer[offset + 2] = muted.b;
            mutedBuffer[offset + 3] = 255;

            offset += 4U;
        }
    }
}

const char* BackgroundArt::getSceneName(ArtScene scene) noexcept
{
    switch (scene) {
    case ArtScene::CyberpunkSkyline:
        return "Cyberpunk Skyline";
    case ArtScene::SynthwaveSunset:
        return "Synthwave Sunset";
    case ArtScene::CosmicNebula:
        return "Cosmic Nebula";
    case ArtScene::LaserMandala:
        return "Laser Mandala";
    default:
        return "Unknown Scene";
    }
}

ArtScene BackgroundArt::getSceneForLevel(std::uint8_t level) noexcept
{
    const auto raw = (level > 0U) ? (level - 1U) : 0U;
    return fromIndex(static_cast<int>(raw % SceneCount));
}

ArtScene BackgroundArt::fromIndex(int index) noexcept
{
    int mod = index % static_cast<int>(SceneCount);
    if (mod < 0) {
        mod += static_cast<int>(SceneCount);
    }
    return static_cast<ArtScene>(mod);
}

// ----------------------------------------------------------------------------
// Scene 0: Cyberpunk Skyline
// ----------------------------------------------------------------------------
PaletteColor BackgroundArt::sampleCyberpunk(float u, float v) noexcept
{
    // 1. Sky Gradient (Deep midnight navy -> royal purple -> dusk magenta)
    PaletteColor skyTop {10, 8, 28, 255};
    PaletteColor skyMid {42, 12, 64, 255};
    PaletteColor skyHorizon {140, 30, 85, 255};

    PaletteColor col;
    if (v < 0.40f) {
        col = lerpColor(skyTop, skyMid, v / 0.40f);
    } else {
        col = lerpColor(skyMid, skyHorizon, (v - 0.40f) / 0.25f);
    }

    // Distant searchlight beam cutting across the sky
    const float beamDist = std::abs((u * 0.7f + v * 0.7f) - 0.42f);
    if (beamDist < 0.035f && v < 0.55f) {
        const float beamIntensity = (1.0f - (beamDist / 0.035f)) * (1.0f - v / 0.55f) * 0.35f;
        col = addGlow(col, 0.0f, 240.0f, 255.0f, beamIntensity);
    }

    // 2. Distant Building Silhouette Layer (v > 0.35)
    constexpr float FarBuildingCount = 18.0f;
    const float farColIdx = std::floor(u * FarBuildingCount);
    const float farColFrac = (u * FarBuildingCount) - farColIdx;
    const float farHeight = 0.35f + hash11(farColIdx * 3.17f) * 0.22f;

    if (v >= farHeight && farColFrac > 0.06f && farColFrac < 0.94f) {
        col = PaletteColor {26, 12, 48, 255};
        // Occasional faint window glow
        if (v > farHeight + 0.03f) {
            const float winY = std::floor(v * 45.0f);
            if (hash21(farColIdx, winY) > 0.75f) {
                col = PaletteColor {80, 50, 110, 255};
            }
        }
    }

    // 3. Mid-ground Skyscraper Layer (v > 0.50)
    constexpr float MidBuildingCount = 12.0f;
    const float midColIdx = std::floor((u + 0.04f) * MidBuildingCount);
    const float midColFrac = ((u + 0.04f) * MidBuildingCount) - midColIdx;
    const float midHeight = 0.48f + hash11(midColIdx * 7.41f + 1.2f) * 0.25f;

    if (v >= midHeight && midColFrac > 0.08f && midColFrac < 0.92f) {
        col = PaletteColor {14, 6, 28, 255};
        // Crisp illuminated windows (Cyan & Gold)
        const float winGridX = std::floor(midColFrac * 6.0f);
        const float winGridY = std::floor(v * 35.0f);
        const float winFracX = (midColFrac * 6.0f) - winGridX;
        const float winFracY = (v * 35.0f) - winGridY;

        if (winFracX > 0.25f && winFracX < 0.75f && winFracY > 0.3f && winFracY < 0.75f) {
            const float winRnd = hash21(midColIdx * 10.0f + winGridX, winGridY);
            if (winRnd > 0.65f) {
                if (winRnd > 0.85f) {
                    col = PaletteColor {0, 240, 255, 255}; // Electric Cyan
                } else {
                    col = PaletteColor {255, 205, 50, 255}; // Neon Amber
                }
            }
        }
    }

    // 4. Foreground City Blocks & Neon Skyline Trim (v > 0.75)
    constexpr float ForeBuildingCount = 8.0f;
    const float foreColIdx = std::floor((u + 0.09f) * ForeBuildingCount);
    const float foreColFrac = ((u + 0.09f) * ForeBuildingCount) - foreColIdx;
    const float foreHeight = 0.72f + hash11(foreColIdx * 11.13f + 4.5f) * 0.16f;

    if (v >= foreHeight && foreColFrac > 0.05f && foreColFrac < 0.95f) {
        col = PaletteColor {5, 2, 14, 255}; // Deep shadow silhouette

        // Neon edge outline along roofline
        if (v - foreHeight < 0.008f) {
            col = (foreColIdx / 2.0f == std::floor(foreColIdx / 2.0f))
                ? PaletteColor {255, 0, 140, 255} // Hot Pink roofline
                : PaletteColor {0, 255, 200, 255}; // Cyan roofline
        }
    }

    // Base road/water reflection at bottom
    if (v > 0.92f) {
        const float wave = std::sin(u * 60.0f + v * 40.0f) * 0.5f + 0.5f;
        col = lerpColor(col, PaletteColor {30, 8, 50, 255}, wave * 0.5f);
    }

    return col;
}

// ----------------------------------------------------------------------------
// Scene 1: Synthwave Sunset
// ----------------------------------------------------------------------------
PaletteColor BackgroundArt::sampleSynthwave(float u, float v) noexcept
{
    constexpr float HorizonY = 0.56f;

    if (v <= HorizonY) {
        // --- Sky & Outrun Sun ---
        // Sky Dusk Gradient
        PaletteColor skyZenith {15, 2, 40, 255};
        PaletteColor skyMid {90, 10, 80, 255};
        PaletteColor skyLow {230, 35, 115, 255};

        PaletteColor col;
        if (v < HorizonY * 0.5f) {
            col = lerpColor(skyZenith, skyMid, v / (HorizonY * 0.5f));
        } else {
            col = lerpColor(skyMid, skyLow, (v - HorizonY * 0.5f) / (HorizonY * 0.5f));
        }

        // Outrun Segmented Sun
        constexpr float SunRadius = 0.22f;
        const float dx = (u - 0.5f) * 1.33f; // Aspect ratio adjustment
        const float dy = v - (HorizonY - 0.02f);
        const float dist = std::sqrt(dx * dx + dy * dy);

        if (dist <= SunRadius) {
            // Sun vertical gradient (Bright yellow at top to blazing red/orange at bottom)
            const float sunT = clamp01((dy + SunRadius) / (SunRadius * 2.0f));
            PaletteColor sunTop {255, 240, 80, 255};
            PaletteColor sunBottom {255, 60, 30, 255};
            PaletteColor sunCol = lerpColor(sunTop, sunBottom, sunT);

            // Horizontal cutoff stripes in lower half of sun
            bool inCutout = false;
            if (dy > -0.04f) {
                const float stripeFreq = 26.0f;
                const float stripePhase = std::fmod((dy + 0.04f) * stripeFreq, 1.0f);
                const float gapThickness = 0.15f + (dy / SunRadius) * 0.55f;
                if (stripePhase < gapThickness) {
                    inCutout = true;
                }
            }

            if (!inCutout) {
                // Sun corona glow
                col = sunCol;
            }
        } else if (dist < SunRadius * 1.4f) {
            // Atmospheric sun corona glow
            const float glow = (1.0f - ((dist - SunRadius) / (SunRadius * 0.4f))) * 0.35f;
            col = addGlow(col, 255.0f, 100.0f, 40.0f, glow);
        }

        // Distant mountain silhouette along horizon
        const float mtnHeight = HorizonY - (std::sin(u * 14.0f) * 0.035f + std::sin(u * 28.0f + 1.2f) * 0.02f);
        if (v >= mtnHeight) {
            col = PaletteColor {20, 4, 35, 255}; // Deep purple mountain
        }

        return col;
    }

    // --- Perspective Wireframe Ground Grid (v > HorizonY) ---
    const float depth = (v - HorizonY) / (1.0f - HorizonY); // [0.0, 1.0]
    const float z = 1.0f / (depth * depth + 0.035f);

    PaletteColor groundCol {12, 3, 24, 255};

    // Perspective longitudinal lines fanning out
    const float perspectiveX = (u - 0.5f) * z * 0.12f;
    const float lineDistX = std::abs(perspectiveX - std::round(perspectiveX));

    // Perspective horizontal latitudinal bars
    const float gridZ = z * 0.06f;
    const float lineDistZ = std::abs(gridZ - std::round(gridZ));

    const float lineIntensityX = std::max(0.0f, 1.0f - lineDistX * 18.0f);
    const float lineIntensityZ = std::max(0.0f, 1.0f - lineDistZ * 14.0f);

    const float totalGrid = std::min(1.0f, lineIntensityX + lineIntensityZ);

    if (totalGrid > 0.0f) {
        // Glowing Neon Cyan and Magenta wireframe
        const PaletteColor gridColor
            = (depth > 0.4f) ? PaletteColor {0, 240, 255, 255} : PaletteColor {255, 0, 180, 255};
        groundCol = lerpColor(groundCol, gridColor, totalGrid * clamp01(depth * 1.5f));
    }

    // Horizon neon light bleed
    if (depth < 0.12f) {
        const float horizonBleed = (1.0f - (depth / 0.12f)) * 0.6f;
        groundCol = addGlow(groundCol, 255.0f, 40.0f, 140.0f, horizonBleed);
    }

    return groundCol;
}

// ----------------------------------------------------------------------------
// Scene 2: Cosmic Nebula & Starfield
// ----------------------------------------------------------------------------
PaletteColor BackgroundArt::sampleCosmic(float u, float v) noexcept
{
    // Deep dark void background
    PaletteColor col {6, 5, 18, 255};

    // Multi-octave nebula plasma cloud
    const float n1 = std::sin(u * 7.0f + std::cos(v * 8.0f)) * 0.5f + 0.5f;
    const float n2 = std::cos(u * 12.0f - v * 10.0f + n1 * 3.0f) * 0.5f + 0.5f;
    const float n3 = std::sin(u * 19.0f + v * 16.0f) * 0.5f + 0.5f;

    // Nebula color layers: Turquoise + Electric Violet + Magenta
    const float nebulaAlpha = std::pow(n1 * 0.5f + n2 * 0.35f + n3 * 0.15f, 1.8f);
    if (nebulaAlpha > 0.12f) {
        PaletteColor neb1 {0, 180, 210, 255}; // Turquoise
        PaletteColor neb2 {140, 30, 220, 255}; // Electric Violet
        PaletteColor neb3 {240, 35, 120, 255}; // Magenta

        const PaletteColor nebCol = lerpColor(neb1, lerpColor(neb2, neb3, n2), n1);
        col = lerpColor(col, nebCol, std::min(0.85f, nebulaAlpha * 1.1f));
    }

    // Starfield: high-frequency grid hash
    constexpr float StarGridSize = 64.0f;
    const float starCellX = std::floor(u * StarGridSize);
    const float starCellY = std::floor(v * StarGridSize);
    const float starHash = hash21(starCellX, starCellY);

    if (starHash > 0.88f) {
        const float starPosX = (starCellX + hash11(starHash * 4.1f)) / StarGridSize;
        const float starPosY = (starCellY + hash11(starHash * 9.3f)) / StarGridSize;
        const float sDist = std::sqrt((u - starPosX) * (u - starPosX) + (v - starPosY) * (v - starPosY));
        const float sRadius = (starHash > 0.98f) ? 0.007f : 0.0035f;

        if (sDist < sRadius) {
            const float starBr = 1.0f - (sDist / sRadius);
            const PaletteColor starColor = (starHash > 0.95f)
                ? PaletteColor {255, 245, 200, 255} // Gold
                : ((starHash > 0.92f) ? PaletteColor {180, 230, 255, 255} : PaletteColor {255, 255, 255, 255});
            col = lerpColor(col, starColor, starBr);
        }
    }

    // Banded Gas Giant Planet (Upper Right: u=0.74, v=0.28, radius=0.14)
    constexpr float PlanetX = 0.74f;
    constexpr float PlanetY = 0.28f;
    constexpr float PlanetR = 0.14f;

    const float pDx = u - PlanetX;
    const float pDy = v - PlanetY;
    const float pDist = std::sqrt(pDx * pDx + pDy * pDy);

    // Planet Rings (Tilted ellipse)
    const float ringX = pDx * 0.866f + pDy * 0.5f;
    const float ringY = (-pDx * 0.5f + pDy * 0.866f) * 3.2f;
    const float ringDist = std::sqrt(ringX * ringX + ringY * ringY);
    if (ringDist >= 0.17f && ringDist <= 0.28f && (pDist > PlanetR || pDy > 0.0f)) {
        const float ringBand = std::sin(ringDist * 160.0f) * 0.5f + 0.5f;
        col = addGlow(col, 220.0f, 180.0f, 140.0f, 0.45f * ringBand);
    }

    // Planet Sphere Body
    if (pDist <= PlanetR) {
        // Atmospheric Bands
        const float band = std::sin((pDy / PlanetR) * 22.0f) * 0.5f + 0.5f;
        PaletteColor planetDark {110, 45, 25, 255}; // Copper/Rust
        PaletteColor planetLight {215, 170, 110, 255}; // Cream Amber
        PaletteColor pCol = lerpColor(planetDark, planetLight, band);

        // Spherical shadow/lighting (Sun from upper left)
        const float nx = pDx / PlanetR;
        const float ny = pDy / PlanetR;
        const float nz = std::sqrt(std::max(0.0f, 1.0f - nx * nx - ny * ny));
        const float light = std::max(0.05f, -nx * 0.6f - ny * 0.5f + nz * 0.62f);

        col = PaletteColor {toByte(static_cast<float>(pCol.r) * light), toByte(static_cast<float>(pCol.g) * light),
            toByte(static_cast<float>(pCol.b) * light), 255};

        // Outer atmospheric glow rim
        if (pDist > PlanetR * 0.92f) {
            col = addGlow(col, 0.0f, 180.0f, 255.0f, 0.4f);
        }
    }

    return col;
}

// ----------------------------------------------------------------------------
// Scene 3: Laser Mandala / Cyber Matrix
// ----------------------------------------------------------------------------
PaletteColor BackgroundArt::sampleMandala(float u, float v) noexcept
{
    // Centered polar coordinates
    const float cx = (u - 0.5f) * 1.33f;
    const float cy = v - 0.5f;
    const float r = std::sqrt(cx * cx + cy * cy);
    const float angle = std::atan2(cy, cx); // [-Pi, Pi]

    // Deep matrix backdrop
    PaletteColor col {4, 10, 14, 255};

    // Subtle background matrix square grid
    const float gridLineX = std::abs((u * 32.0f) - std::round(u * 32.0f));
    const float gridLineY = std::abs((v * 32.0f) - std::round(v * 32.0f));
    if (gridLineX < 0.06f || gridLineY < 0.06f) {
        col = PaletteColor {8, 28, 32, 255};
    }

    // Concentric Laser Rings (8-fold and 16-fold kaleidoscopic geometry)
    constexpr float RingCount = 6.0f;
    for (float i {1.0f}; i <= RingCount; i += 1.0f) {
        const float targetR = i * 0.075f;
        const float distR = std::abs(r - targetR);

        // Circular ring glow
        if (distR < 0.012f) {
            const float intensity = (1.0f - (distR / 0.012f));
            if (static_cast<int>(i) % 2 == 0) {
                col = addGlow(col, 0.0f, 255.0f, 220.0f, intensity * 0.9f); // Electric Cyan
            } else {
                col = addGlow(col, 255.0f, 20.0f, 180.0f, intensity * 0.9f); // Hot Magenta
            }
        }

        // Geometric Petals / Spikes (8-fold symmetry)
        const float symmetry = 8.0f * i;
        const float petal = std::cos(angle * symmetry);
        const float petalR = targetR + petal * 0.022f;
        const float petalDist = std::abs(r - petalR);

        if (petalDist < 0.008f) {
            const float pIntensity = (1.0f - (petalDist / 0.008f));
            col = addGlow(col, 240.0f, 255.0f, 40.0f, pIntensity * 0.75f); // Neon Yellow
        }
    }

    // Central Core Pulse Star
    if (r < 0.045f) {
        const float coreDist = r / 0.045f;
        const float coreAlpha = 1.0f - coreDist;
        col = addGlow(col, 255.0f, 255.0f, 255.0f, coreAlpha * 1.2f);
    }

    return col;
}

} // namespace qix
