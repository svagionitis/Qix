#pragma once
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
};

} // namespace qix
