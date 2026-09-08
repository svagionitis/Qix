#pragma once
#include "Types.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace qix {

/// @brief Represents a single high score record in the Hall of Fame.
struct HighScoreEntry {
    std::string initials {"AAA"};
    std::uint32_t score {0};
    std::uint8_t level {1};
    GameMode mode {DefaultGameMode};

    [[nodiscard]] constexpr bool operator==(const HighScoreEntry& other) const noexcept
    {
        return score == other.score && level == other.level && mode == other.mode && initials == other.initials;
    }

    [[nodiscard]] constexpr bool operator!=(const HighScoreEntry& other) const noexcept
    {
        return !(*this == other);
    }
};

/// @class HighScoreTable
/// @brief Manages the Arcade Hall of Fame leaderboard and disk persistence.
/// @details Maintains a descending sorted list of top player entries with fixed capacity.
class HighScoreTable {
public:
    static constexpr std::size_t DefaultCapacity {10};

    /// @brief Construct high score table with given maximum capacity.
    /// @param[in] capacity Maximum entries to retain (default: 10).
    explicit HighScoreTable(std::size_t capacity = DefaultCapacity) noexcept;

    /// @brief Check if a score qualifies for insertion into the leaderboard.
    /// @param[in] score Candidate score.
    /// @return True if score is eligible for leaderboard.
    [[nodiscard]] bool qualifies(std::uint32_t score) const noexcept;

    /// @brief Determine the 1-based rank a score would achieve.
    /// @param[in] score Candidate score.
    /// @return 1-based rank position, or 0 if not qualified.
    [[nodiscard]] std::size_t getRank(std::uint32_t score) const noexcept;

    /// @brief Insert a new score record into the leaderboard.
    /// @param[in] initials 3-letter player initials.
    /// @param[in] score Final score value.
    /// @param[in] level Level reached.
    /// @param[in] mode Game ruleset mode.
    /// @return True if record was inserted into the table.
    [[nodiscard]] bool insert(
        std::string_view initials, std::uint32_t score, std::uint8_t level, GameMode mode) noexcept;

    /// @brief Retrieve immutable view of current leaderboard entries.
    /// @return Reference to vector of HighScoreEntry records.
    [[nodiscard]] const std::vector<HighScoreEntry>& getEntries() const noexcept;

    /// @brief Get current number of stored records.
    /// @return Number of entries.
    [[nodiscard]] std::size_t size() const noexcept;

    /// @brief Get maximum entry capacity.
    /// @return Table capacity.
    [[nodiscard]] std::size_t capacity() const noexcept;

    /// @brief Retrieve the current all-time highest score.
    /// @return Highest score in table, or 0 if empty.
    [[nodiscard]] std::uint32_t getHighScore() const noexcept;

    /// @brief Reset leaderboard to classic 1980s arcade default records.
    void resetToDefaults() noexcept;

    /// @brief Load leaderboard records from a file on disk.
    /// @param[in] filepath Absolute or relative path to score file.
    /// @return True if loaded successfully.
    [[nodiscard]] bool loadFromFile(const std::string& filepath) noexcept;

    /// @brief Save leaderboard records to a file on disk.
    /// @param[in] filepath Absolute or relative path to score file.
    /// @return True if saved successfully.
    [[nodiscard]] bool saveToFile(const std::string& filepath) const noexcept;

    /// @brief Determine default cross-platform high scores filepath.
    /// @return Path string to default storage file.
    [[nodiscard]] static std::string getDefaultFilePath() noexcept;

    /// @brief Sanitize raw string to valid 3-character uppercase initials.
    /// @param[in] raw Raw input string.
    /// @return Formatted 3-character uppercase string.
    [[nodiscard]] static std::string sanitizeInitials(std::string_view raw) noexcept;

private:
    std::size_t m_capacity {DefaultCapacity};
    std::vector<HighScoreEntry> m_entries {};
};

} // namespace qix
