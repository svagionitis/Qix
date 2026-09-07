#include "HighScoreTable.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>

namespace qix {

HighScoreTable::HighScoreTable(std::size_t capacity) noexcept
    : m_capacity {capacity > 0 ? capacity : DefaultCapacity}
{
    resetToDefaults();
}

bool HighScoreTable::qualifies(std::uint32_t score) const noexcept
{
    if (score == 0) {
        return false;
    }

    if (m_entries.size() < m_capacity) {
        return true;
    }

    return score > m_entries.back().score;
}

std::size_t HighScoreTable::getRank(std::uint32_t score) const noexcept
{
    if (!qualifies(score)) {
        return 0;
    }

    for (std::size_t i {0}; i < m_entries.size(); ++i) {
        if (score > m_entries[i].score) {
            return i + 1;
        }
    }

    return m_entries.size() < m_capacity ? m_entries.size() + 1 : 0;
}

bool HighScoreTable::insert(std::string_view initials, std::uint32_t score, std::uint8_t level, GameMode mode) noexcept
{
    if (!qualifies(score)) {
        return false;
    }

    HighScoreEntry entry {};
    entry.initials = sanitizeInitials(initials);
    entry.score = score;
    entry.level = level;
    entry.mode = mode;

    auto it = std::upper_bound(m_entries.begin(), m_entries.end(), entry,
        [](const HighScoreEntry& lhs, const HighScoreEntry& rhs) { return lhs.score > rhs.score; });

    m_entries.insert(it, entry);

    if (m_entries.size() > m_capacity) {
        m_entries.resize(m_capacity);
    }

    return true;
}

const std::vector<HighScoreEntry>& HighScoreTable::getEntries() const noexcept
{
    return m_entries;
}

std::size_t HighScoreTable::size() const noexcept
{
    return m_entries.size();
}

std::size_t HighScoreTable::capacity() const noexcept
{
    return m_capacity;
}

std::uint32_t HighScoreTable::getHighScore() const noexcept
{
    return m_entries.empty() ? 0U : m_entries.front().score;
}

void HighScoreTable::resetToDefaults() noexcept
{
    m_entries.clear();

    const std::vector<HighScoreEntry> defaults {
        HighScoreEntry {"QIX", 50000U, 5, GameMode::Classic},
        HighScoreEntry {"TAI", 40000U, 4, GameMode::Classic},
        HighScoreEntry {"TOA", 30000U, 3, GameMode::Classic},
        HighScoreEntry {"ARC", 20000U, 2, GameMode::Classic},
        HighScoreEntry {"STV", 15000U, 2, GameMode::Classic},
        HighScoreEntry {"BOB", 10000U, 1, GameMode::Classic},
        HighScoreEntry {"DAN", 7500U, 1, GameMode::Classic},
        HighScoreEntry {"KEN", 5000U, 1, GameMode::Classic},
        HighScoreEntry {"RON", 2500U, 1, GameMode::Classic},
        HighScoreEntry {"JOE", 1000U, 1, GameMode::Classic},
    };

    const std::size_t count = std::min(m_capacity, defaults.size());
    m_entries.assign(defaults.begin(), defaults.begin() + static_cast<std::ptrdiff_t>(count));
}

bool HighScoreTable::loadFromFile(const std::string& filepath) noexcept
{
    std::ifstream file {filepath};
    if (!file.is_open()) {
        return false;
    }

    std::vector<HighScoreEntry> loaded {};
    std::string line {};

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss {line};
        std::string rawInitials {};
        std::uint32_t score {0};
        std::uint32_t rawLevel {1};
        std::uint32_t rawMode {0};

        if (iss >> rawInitials >> score >> rawLevel >> rawMode) {
            HighScoreEntry entry {};
            entry.initials = sanitizeInitials(rawInitials);
            entry.score = score;
            entry.level = static_cast<std::uint8_t>(std::clamp(rawLevel, 1U, 99U));
            entry.mode = (rawMode == 1U) ? GameMode::Modern : GameMode::Classic;
            loaded.push_back(entry);
        }
    }

    if (loaded.empty()) {
        return false;
    }

    std::stable_sort(loaded.begin(), loaded.end(),
        [](const HighScoreEntry& a, const HighScoreEntry& b) { return a.score > b.score; });

    if (loaded.size() > m_capacity) {
        loaded.resize(m_capacity);
    }

    m_entries = std::move(loaded);
    return true;
}

bool HighScoreTable::saveToFile(const std::string& filepath) const noexcept
{
    std::ofstream file {filepath};
    if (!file.is_open()) {
        return false;
    }

    file << "# Qix Arcade Hall of Fame\n";
    for (const auto& entry : m_entries) {
        file << entry.initials << ' ' << entry.score << ' ' << static_cast<std::uint32_t>(entry.level) << ' '
             << static_cast<std::uint32_t>(entry.mode) << '\n';
    }

    return true;
}

std::string HighScoreTable::getDefaultFilePath() noexcept
{
    const char* home = std::getenv("HOME");
    if (home != nullptr && *home != '\0') {
        return std::string {home} + "/.qix_highscores.txt";
    }

    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile != nullptr && *userProfile != '\0') {
        return std::string {userProfile} + "/.qix_highscores.txt";
    }

    return ".qix_highscores.txt";
}

std::string HighScoreTable::sanitizeInitials(std::string_view raw) noexcept
{
    std::string result {};
    result.reserve(3);

    for (const char ch : raw) {
        if (result.size() >= 3) {
            break;
        }

        const auto uc = static_cast<unsigned char>(ch);
        if (std::isalnum(uc) != 0 || ch == '!' || ch == '.' || ch == '?') {
            result.push_back(static_cast<char>(std::toupper(uc)));
        }
    }

    while (result.size() < 3) {
        result.push_back('A');
    }

    return result;
}

} // namespace qix
