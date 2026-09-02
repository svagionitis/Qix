#ifndef QIX_LIB_SPEED_CONFIG_H
#define QIX_LIB_SPEED_CONFIG_H

#include <cstdint>
#include <string>
#include <vector>

namespace qix {

/// @class SpeedConfig
/// @brief Centralized pacing constants, boundary constraints, and argument parsing logic.
/// @details Ensures uniform speed handling across terminal and graphical clients.
class SpeedConfig {
public:
    /// @brief Default simulation tick delay in milliseconds (~13.3 FPS).
    static constexpr std::uint32_t DefaultDelayMs {75U};

    /// @brief Minimum allowable tick delay in milliseconds (50 FPS).
    static constexpr std::uint32_t MinDelayMs {20U};

    /// @brief Maximum allowable tick delay in milliseconds (4 FPS).
    static constexpr std::uint32_t MaxDelayMs {250U};

    /// @brief Incremental step in milliseconds when adjusting speed at runtime.
    static constexpr std::uint32_t StepDelayMs {10U};

    /// @brief Clamp candidate delay in milliseconds to [MinDelayMs, MaxDelayMs].
    /// @param[in] delayMs Raw delay in milliseconds.
    /// @return Clamped tick delay in milliseconds.
    [[nodiscard]] static constexpr std::uint32_t clampDelay(std::uint32_t delayMs) noexcept
    {
        return (delayMs < MinDelayMs) ? MinDelayMs : ((delayMs > MaxDelayMs) ? MaxDelayMs : delayMs);
    }

    /// @brief Compute updated delay when increasing game speed (lowering tick delay).
    /// @param[in] currentDelay Current tick delay in milliseconds.
    /// @return Decreased delay in milliseconds clamped to MinDelayMs.
    [[nodiscard]] static constexpr std::uint32_t speedUp(std::uint32_t currentDelay) noexcept
    {
        return (currentDelay > MinDelayMs + StepDelayMs) ? (currentDelay - StepDelayMs) : MinDelayMs;
    }

    /// @brief Compute updated delay when decreasing game speed (increasing tick delay).
    /// @param[in] currentDelay Current tick delay in milliseconds.
    /// @return Increased delay in milliseconds clamped to MaxDelayMs.
    [[nodiscard]] static constexpr std::uint32_t speedDown(std::uint32_t currentDelay) noexcept
    {
        return (currentDelay + StepDelayMs > MaxDelayMs) ? MaxDelayMs : (currentDelay + StepDelayMs);
    }

    /// @brief Parse speed settings from command line arguments (--delay/-d or --fps/-f).
    /// @param[in] argc Argument count.
    /// @param[in] argv Argument array.
    /// @param[in] defaultDelay Initial delay fallback in milliseconds.
    /// @return Clamped delay in milliseconds.
    [[nodiscard]] static std::uint32_t parseSpeedArgs(
        int argc, char* const argv[], std::uint32_t defaultDelay = DefaultDelayMs) noexcept;

    /// @brief Parse speed settings from a string vector of arguments.
    /// @param[in] args Vector of command-line argument strings.
    /// @param[in] defaultDelay Initial delay fallback in milliseconds.
    /// @return Clamped delay in milliseconds.
    [[nodiscard]] static std::uint32_t parseSpeedArgs(
        const std::vector<std::string>& args, std::uint32_t defaultDelay = DefaultDelayMs) noexcept;
};

} // namespace qix

#endif // QIX_LIB_SPEED_CONFIG_H
