#pragma once
#include "ColorPalette.h"
#include "Types.h"
#include <string>
#include <vector>

namespace qix {

/// @class GameConfig
/// @brief Centralized command-line argument parsing for gameplay mode and options.
/// @details Supports --classic, --modern, and --mode/-m <classic|modern> flags across all clients.
class GameConfig {
public:
    /// @brief Parse game mode from command line arguments (--classic, --modern, --mode/-m <classic|modern>).
    /// @param[in] argc Argument count.
    /// @param[in] argv Argument array.
    /// @param[in] defaultMode Fallback mode if flag is absent.
    /// @return Configured GameMode.
    [[nodiscard]] static GameMode parseGameMode(
        int argc, char* const argv[], GameMode defaultMode = DefaultGameMode) noexcept;

    /// @brief Parse game mode from a vector of argument strings.
    /// @param[in] args Vector of command line argument strings.
    /// @param[in] defaultMode Fallback mode if flag is absent.
    /// @return Configured GameMode.
    [[nodiscard]] static GameMode parseGameMode(
        const std::vector<std::string>& args, GameMode defaultMode = DefaultGameMode) noexcept;

    /// @brief Convert GameMode enum to human-readable string representation.
    /// @param[in] mode GameMode value.
    /// @return String representation ("Classic" or "Modern").
    [[nodiscard]] static const char* toString(GameMode mode) noexcept;

    /// @brief Parse CRT scanlines and phosphor glow flag from command line arguments (--crt, --scanlines, -c,
    /// --no-crt).
    /// @param[in] argc Argument count.
    /// @param[in] argv Argument array.
    /// @param[in] defaultCrt Fallback value if flag is absent (default: false).
    /// @return True if CRT filter is enabled, false otherwise.
    [[nodiscard]] static bool parseCrtFlag(int argc, char* const argv[], bool defaultCrt = false) noexcept;

    /// @brief Parse CRT scanlines and phosphor glow flag from a vector of argument strings.
    /// @param[in] args Vector of command line argument strings.
    /// @param[in] defaultCrt Fallback value if flag is absent (default: false).
    /// @return True if CRT filter is enabled, false otherwise.
    [[nodiscard]] static bool parseCrtFlag(const std::vector<std::string>& args, bool defaultCrt = false) noexcept;

    /// @brief Parse audio sound synthesis flag from command line arguments (--audio, --sound, -s, --no-audio,
    /// --no-sound).
    /// @param[in] argc Argument count.
    /// @param[in] argv Argument array.
    /// @param[in] defaultAudio Fallback value if flag is absent (default: false).
    /// @return True if audio output is enabled, false otherwise.
    [[nodiscard]] static bool parseAudioFlag(int argc, char* const argv[], bool defaultAudio = false) noexcept;

    /// @brief Parse audio sound synthesis flag from a vector of argument strings.
    /// @param[in] args Vector of command line argument strings.
    /// @param[in] defaultAudio Fallback value if flag is absent (default: false).
    /// @return True if audio output is enabled, false otherwise.
    [[nodiscard]] static bool parseAudioFlag(const std::vector<std::string>& args, bool defaultAudio = false) noexcept;

    /// @brief Parse color palette from command line arguments (--palette=<name>, -p <name>).
    /// @param[in] argc Argument count.
    /// @param[in] argv Argument array.
    /// @param[in] defaultPalette Fallback palette if flag is absent (default: PaletteId::Classic).
    /// @return Configured PaletteId.
    [[nodiscard]] static PaletteId parsePaletteFlag(
        int argc, char* const argv[], PaletteId defaultPalette = PaletteId::Classic) noexcept;

    /// @brief Parse color palette from a vector of argument strings.
    /// @param[in] args Vector of command line argument strings.
    /// @param[in] defaultPalette Fallback palette if flag is absent (default: PaletteId::Classic).
    /// @return Configured PaletteId.
    [[nodiscard]] static PaletteId parsePaletteFlag(
        const std::vector<std::string>& args, PaletteId defaultPalette = PaletteId::Classic) noexcept;

    /// @brief Parse attract / demo mode flag from command line arguments (--attract, --demo, -d).
    /// @param[in] argc Argument count.
    /// @param[in] argv Argument array.
    /// @param[in] defaultAttract Fallback value if flag is absent (default: false).
    /// @return True if attract mode should start immediately, false otherwise.
    [[nodiscard]] static bool parseAttractFlag(int argc, char* const argv[], bool defaultAttract = false) noexcept;

    /// @brief Parse attract / demo mode flag from a vector of argument strings.
    /// @param[in] args Vector of command line argument strings.
    /// @param[in] defaultAttract Fallback value if flag is absent (default: false).
    /// @return True if attract mode should start immediately, false otherwise.
    [[nodiscard]] static bool parseAttractFlag(const std::vector<std::string>& args, bool defaultAttract = false) noexcept;

    /// @brief Parse background art reveal mode flag (--art, --bg-art, --reveal, --no-art, --no-bg-art).
    /// @param[in] argc Argument count.
    /// @param[in] argv Argument array.
    /// @param[in] defaultArt Fallback value if flag is absent (default: true).
    /// @return True if background art reveal mode is enabled, false otherwise.
    [[nodiscard]] static bool parseArtFlag(int argc, char* const argv[], bool defaultArt = true) noexcept;

    /// @brief Parse background art reveal mode flag from a vector of argument strings.
    /// @param[in] args Vector of command line argument strings.
    /// @param[in] defaultArt Fallback value if flag is absent (default: true).
    /// @return True if background art reveal mode is enabled, false otherwise.
    [[nodiscard]] static bool parseArtFlag(const std::vector<std::string>& args, bool defaultArt = true) noexcept;

    /// @brief Parse background art scene index (--art-scene=<0..3>, --scene <0..3>).
    /// @param[in] argc Argument count.
    /// @param[in] argv Argument array.
    /// @param[in] defaultScene Fallback scene index if flag is absent (default: -1 for auto level-based).
    /// @return Scene index (0..3) or -1 for auto level-based cycling.
    [[nodiscard]] static int parseArtSceneFlag(int argc, char* const argv[], int defaultScene = -1) noexcept;

    /// @brief Parse background art scene index from a vector of argument strings.
    /// @param[in] args Vector of command line argument strings.
    /// @param[in] defaultScene Fallback scene index if flag is absent (default: -1 for auto level-based).
    /// @return Scene index (0..3) or -1 for auto level-based cycling.
    [[nodiscard]] static int parseArtSceneFlag(const std::vector<std::string>& args, int defaultScene = -1) noexcept;
};

} // namespace qix
