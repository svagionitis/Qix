#pragma once
#include "ColorPalette.h"
#include "HighScoreTable.h"
#include "Types.h"
#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace qix {

/// @struct InstructionRule
/// @brief A single how-to-play instruction rule card entry.
struct InstructionRule {
    const char* header;
    const char* detail;
    PaletteColor color;
};

/// @struct VictoryPresentation
/// @brief Formatted view-model for Level Complete, Qix Trap, and Split Bonus overlays.
struct VictoryPresentation {
    std::string title;
    std::string detail;
    std::string bonus;
    std::string prompt;
    PaletteColor titleColor;
    bool hasBonus {false};
    bool hasDetail {false};
    bool isTrap {false};
};

/// @enum HudUrgency
/// @brief Urgency tier for time countdown display in the HUD.
enum class HudUrgency : std::uint8_t { Normal, Warning, Critical };

/// @struct HudPresentation
/// @brief Formatted view-model for the top HUD status bar.
struct HudPresentation {
    std::string scoreStr;
    std::string hiScoreStr;
    std::string claimStr;
    bool targetReached {false};
    float progressRatio {0.0f};
    int fillPercent {0};
    std::uint32_t secondsRemaining {0};
    std::string timeStr;
    HudUrgency timeUrgency {HudUrgency::Normal};
    std::string multiplierStr;
    std::string speedStr;
    std::string levelStr;
};

/// @enum MedalTier
/// @brief Hall of Fame ranking achievement badge tiers.
enum class MedalTier : std::uint8_t { Gold = 0, Silver = 1, Bronze = 2, Standard = 3 };

/// @struct HofRowPresentation
/// @brief Formatted row in the Hall of Fame table.
struct HofRowPresentation {
    std::size_t rank {0};
    std::string formattedRow;
    MedalTier medal {MedalTier::Standard};
    PaletteColor medalColor;
};

/// @class GamePresenter
/// @brief Centralized, zero-allocation presenter generating view-models for frontends.
/// @details Consolidates game status formatting, victory fanfare messages, instruction cards,
/// and arcade Hall of Fame row layouts to prevent logic duplication across clients.
class GamePresenter {
public:
    /// @brief Retrieve the standard 5-rule instructions text array.
    /// @return Reference to static array of 5 InstructionRule entries.
    [[nodiscard]] static const std::array<InstructionRule, 5>& getInstructionRules() noexcept;

    /// @brief Format game stats into a victory presentation model for Level Complete states.
    /// @param[in] stats Active GameStats snapshot.
    /// @return Formatted VictoryPresentation struct.
    [[nodiscard]] static VictoryPresentation formatVictory(const GameStats& stats) noexcept;

    /// @brief Format game statistics into HUD strings and status indicators.
    /// @param[in] stats Active GameStats snapshot.
    /// @param[in] delayMs Current loop step delay in milliseconds.
    /// @return Formatted HudPresentation struct.
    [[nodiscard]] static HudPresentation formatHud(const GameStats& stats, std::uint32_t delayMs) noexcept;

    /// @brief Format high score table entries into Hall of Fame rows.
    /// @param[in] table HighScoreTable pointer (can be nullptr).
    /// @param[in] maxRows Maximum number of rows to format (default 7).
    /// @return Vector of formatted HofRowPresentation structs.
    [[nodiscard]] static std::vector<HofRowPresentation> formatHallOfFame(
        const HighScoreTable* table, std::size_t maxRows = 7) noexcept;

    /// @brief Generate the blinking arcade prompt string for Attract or Game Over.
    /// @param[in] isAttract True if in Attract demo mode.
    /// @param[in] blink True if blink cycle is currently visible.
    /// @return Prompt text string (or empty if blinking off).
    [[nodiscard]] static std::string formatHofPrompt(bool isAttract, bool blink) noexcept;

    /// @brief Format a new record banner message for Name Entry state.
    /// @param[in] rank Leaderboard position (1-indexed).
    /// @param[in] score Player's final score.
    /// @return Formatted record banner string.
    [[nodiscard]] static std::string formatRecordBanner(std::size_t rank, std::uint32_t score) noexcept;
};

} // namespace qix
