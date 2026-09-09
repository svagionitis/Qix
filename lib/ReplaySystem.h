#pragma once
#include "Types.h"
#include <cstdint>
#include <string>
#include <vector>

namespace qix {

/// @struct ReplayHeader
/// @brief Metadata and initial simulation parameters required to reproduce a game run.
struct ReplayHeader {
    std::uint32_t version {1};
    GameMode mode {DefaultGameMode};
    std::int32_t playfieldWidth {80};
    std::int32_t playfieldHeight {60};
    std::uint16_t targetPercent {75};
    std::uint32_t baseDelayMs {75};
    std::uint32_t seed {1981};
    std::uint32_t totalTicks {0};
    std::uint32_t finalScore {0};
    std::string date {};
};

/// @struct ReplayFrame
/// @brief Discrete input command captured at a specific simulation tick.
struct ReplayFrame {
    std::uint32_t tick {0};
    PlayerCommand cmd {};
};

/// @class ReplaySystem
/// @brief Serialization and deserialization utilities for deterministic replay streams.
class ReplaySystem {
public:
    /// @brief Save replay data to a file (JSON or compact format depending on extension).
    /// @param[in] filepath Destination file path.
    /// @param[in] header Replay header metadata and game configuration.
    /// @param[in] frames Sequence of recorded input frames.
    /// @return True if file saved successfully, false on error.
    [[nodiscard]] static bool saveToFile(
        const std::string& filepath, const ReplayHeader& header, const std::vector<ReplayFrame>& frames) noexcept;

    /// @brief Load replay data from a file, auto-detecting JSON or compact text format.
    /// @param[in] filepath Source file path.
    /// @param[out] outHeader Loaded replay header metadata.
    /// @param[out] outFrames Loaded recorded input frames.
    /// @return True if file was parsed successfully, false on error or corruption.
    [[nodiscard]] static bool loadFromFile(
        const std::string& filepath, ReplayHeader& outHeader, std::vector<ReplayFrame>& outFrames) noexcept;

    /// @brief Serialize replay header and frames into structured JSON string.
    /// @param[in] header Replay header metadata.
    /// @param[in] frames Recorded input frames.
    /// @return Serialized JSON string.
    [[nodiscard]] static std::string serializeJson(
        const ReplayHeader& header, const std::vector<ReplayFrame>& frames) noexcept;

    /// @brief Parse replay header and frames from a JSON string.
    /// @param[in] json JSON string content.
    /// @param[out] outHeader Loaded header.
    /// @param[out] outFrames Loaded frames.
    /// @return True if JSON parsing succeeded, false otherwise.
    [[nodiscard]] static bool deserializeJson(
        const std::string& json, ReplayHeader& outHeader, std::vector<ReplayFrame>& outFrames) noexcept;

    /// @brief Serialize replay header and frames into compact line-based format.
    /// @param[in] header Replay header metadata.
    /// @param[in] frames Recorded input frames.
    /// @return Serialized compact text string.
    [[nodiscard]] static std::string serializeCompact(
        const ReplayHeader& header, const std::vector<ReplayFrame>& frames) noexcept;

    /// @brief Parse replay header and frames from compact line-based format.
    /// @param[in] text Compact text string content.
    /// @param[out] outHeader Loaded header.
    /// @param[out] outFrames Loaded frames.
    /// @return True if parsing succeeded, false otherwise.
    [[nodiscard]] static bool deserializeCompact(
        const std::string& text, ReplayHeader& outHeader, std::vector<ReplayFrame>& outFrames) noexcept;

    [[nodiscard]] static const char* directionToString(Direction dir) noexcept;
    [[nodiscard]] static Direction stringToDirection(const std::string& str) noexcept;
    [[nodiscard]] static const char* drawModeToString(DrawMode mode) noexcept;
    [[nodiscard]] static DrawMode stringToDrawMode(const std::string& str) noexcept;
    [[nodiscard]] static const char* gameModeToString(GameMode mode) noexcept;
    [[nodiscard]] static GameMode stringToGameMode(const std::string& str) noexcept;
};

/// @class ReplayRecorder
/// @brief In-flight session recorder accumulating input commands per tick.
class ReplayRecorder {
public:
    ReplayRecorder() noexcept = default;

    /// @brief Initialize recording session with game configuration parameters.
    /// @param[in] header Game configuration for the run.
    void start(const ReplayHeader& header) noexcept;

    /// @brief Record player command at the specified simulation tick.
    /// @param[in] tick Discrete simulation tick index.
    /// @param[in] cmd Player command at this tick.
    void recordTick(std::uint32_t tick, PlayerCommand cmd) noexcept;

    /// @brief Finalize recording session with outcome stats.
    /// @param[in] finalScore Final score achieved.
    /// @param[in] totalTicks Total ticks simulated.
    void finish(std::uint32_t finalScore, std::uint32_t totalTicks) noexcept;

    /// @brief Save recording to file.
    /// @param[in] filepath Target path.
    /// @return True if file saved.
    [[nodiscard]] bool saveToFile(const std::string& filepath) const noexcept;

    /// @brief Check whether currently recording.
    /// @return True if active recording session.
    [[nodiscard]] bool isRecording() const noexcept;

    /// @brief Retrieve recorded header.
    /// @return Const reference to ReplayHeader.
    [[nodiscard]] const ReplayHeader& getHeader() const noexcept;

    /// @brief Retrieve recorded frames.
    /// @return Const reference to vector of ReplayFrames.
    [[nodiscard]] const std::vector<ReplayFrame>& getFrames() const noexcept;

    /// @brief Reset and discard current recording.
    void reset() noexcept;

private:
    ReplayHeader m_header {};
    std::vector<ReplayFrame> m_frames {};
    bool m_recording {false};
};

/// @class ReplayPlayer
/// @brief Playback controller providing recorded player commands per simulation tick.
class ReplayPlayer {
public:
    ReplayPlayer() noexcept = default;

    /// @brief Load replay file from disk.
    /// @param[in] filepath Source replay file path.
    /// @return True on success, false on error.
    [[nodiscard]] bool loadFromFile(const std::string& filepath) noexcept;

    /// @brief Load replay from raw JSON string.
    /// @param[in] json Source JSON.
    /// @return True on success, false on error.
    [[nodiscard]] bool loadFromJson(const std::string& json) noexcept;

    /// @brief Load replay from raw compact string.
    /// @param[in] text Source text.
    /// @return True on success, false on error.
    [[nodiscard]] bool loadFromCompact(const std::string& text) noexcept;

    /// @brief Query input command for a given simulation tick.
    /// @param[in] tick Current simulation tick.
    /// @return PlayerCommand recorded for this tick (or default idle command).
    [[nodiscard]] PlayerCommand getCommandForTick(std::uint32_t tick) const noexcept;

    /// @brief Check whether the playback has reached or exceeded total recorded ticks.
    /// @param[in] currentTick Current simulation tick.
    /// @return True if finished.
    [[nodiscard]] bool isFinished(std::uint32_t currentTick) const noexcept;

    /// @brief Check whether a replay is currently loaded and ready for playback.
    /// @return True if replay data loaded.
    [[nodiscard]] bool isLoaded() const noexcept;

    /// @brief Get replay header configuration.
    /// @return Const reference to ReplayHeader.
    [[nodiscard]] const ReplayHeader& getHeader() const noexcept;

    /// @brief Get total recorded ticks.
    /// @return Total ticks in recording.
    [[nodiscard]] std::uint32_t getTotalTicks() const noexcept;

    /// @brief Reset playback state.
    void reset() noexcept;

private:
    ReplayHeader m_header {};
    std::vector<ReplayFrame> m_frames {};
    bool m_loaded {false};
};

} // namespace qix
