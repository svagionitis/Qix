#include "ReplaySystem.h"
#include <algorithm>
#include <fstream>
#include <sstream>

namespace qix {

const char* ReplaySystem::directionToString(Direction dir) noexcept
{
    switch (dir) {
    case Direction::Up:
        return "Up";
    case Direction::Down:
        return "Down";
    case Direction::Left:
        return "Left";
    case Direction::Right:
        return "Right";
    case Direction::None:
    default:
        return "None";
    }
}

Direction ReplaySystem::stringToDirection(const std::string& str) noexcept
{
    if (str == "Up" || str == "up" || str == "U") {
        return Direction::Up;
    }
    if (str == "Down" || str == "down" || str == "D") {
        return Direction::Down;
    }
    if (str == "Left" || str == "left" || str == "L") {
        return Direction::Left;
    }
    if (str == "Right" || str == "right" || str == "R") {
        return Direction::Right;
    }
    return Direction::None;
}

const char* ReplaySystem::drawModeToString(DrawMode mode) noexcept
{
    switch (mode) {
    case DrawMode::Slow:
        return "Slow";
    case DrawMode::Fast:
        return "Fast";
    case DrawMode::None:
    default:
        return "None";
    }
}

DrawMode ReplaySystem::stringToDrawMode(const std::string& str) noexcept
{
    if (str == "Slow" || str == "slow" || str == "S") {
        return DrawMode::Slow;
    }
    if (str == "Fast" || str == "fast" || str == "F") {
        return DrawMode::Fast;
    }
    return DrawMode::None;
}

const char* ReplaySystem::gameModeToString(GameMode mode) noexcept
{
    return (mode == GameMode::Modern) ? "Modern" : "Classic";
}

GameMode ReplaySystem::stringToGameMode(const std::string& str) noexcept
{
    return (str == "Modern" || str == "modern" || str == "1") ? GameMode::Modern : GameMode::Classic;
}

std::string ReplaySystem::serializeCompact(const ReplayHeader& header, const std::vector<ReplayFrame>& frames) noexcept
{
    std::ostringstream oss;
    oss << "# QIX REPLAY RECORDING v1\n";
    oss << "version: " << header.version << "\n";
    oss << "mode: " << gameModeToString(header.mode) << "\n";
    oss << "width: " << header.playfieldWidth << "\n";
    oss << "height: " << header.playfieldHeight << "\n";
    oss << "targetPercent: " << header.targetPercent << "\n";
    oss << "baseDelayMs: " << header.baseDelayMs << "\n";
    oss << "seed: " << header.seed << "\n";
    oss << "totalTicks: " << header.totalTicks << "\n";
    oss << "finalScore: " << header.finalScore << "\n";
    if (!header.date.empty()) {
        oss << "date: " << header.date << "\n";
    }
    oss << "[INPUTS]\n";

    for (const auto& frame : frames) {
        oss << frame.tick << ' ' << directionToString(frame.cmd.direction) << ' '
            << drawModeToString(frame.cmd.drawMode) << '\n';
    }

    return oss.str();
}

bool ReplaySystem::deserializeCompact(
    const std::string& text, ReplayHeader& outHeader, std::vector<ReplayFrame>& outFrames) noexcept
{
    if (text.empty() || text.find("[INPUTS]") == std::string::npos || text.find("version:") == std::string::npos) {
        return false;
    }

    std::istringstream iss(text);
    std::string line;
    bool inInputs = false;
    ReplayHeader hdr {};
    std::vector<ReplayFrame> frames {};

    while (std::getline(iss, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        if (line == "[INPUTS]") {
            inInputs = true;
            continue;
        }

        if (!inInputs) {
            const auto colonPos = line.find(':');
            if (colonPos != std::string::npos) {
                const std::string key = line.substr(0, colonPos);
                std::string val = line.substr(colonPos + 1);
                // Trim leading whitespace
                const auto valStart = val.find_first_not_of(" \t");
                if (valStart != std::string::npos) {
                    val = val.substr(valStart);
                }

                if (key == "version") {
                    hdr.version = static_cast<std::uint32_t>(std::stoul(val));
                } else if (key == "mode") {
                    hdr.mode = stringToGameMode(val);
                } else if (key == "width") {
                    hdr.playfieldWidth = std::stoi(val);
                } else if (key == "height") {
                    hdr.playfieldHeight = std::stoi(val);
                } else if (key == "targetPercent") {
                    hdr.targetPercent = static_cast<std::uint16_t>(std::stoul(val));
                } else if (key == "baseDelayMs") {
                    hdr.baseDelayMs = static_cast<std::uint32_t>(std::stoul(val));
                } else if (key == "seed") {
                    hdr.seed = static_cast<std::uint32_t>(std::stoul(val));
                } else if (key == "totalTicks") {
                    hdr.totalTicks = static_cast<std::uint32_t>(std::stoul(val));
                } else if (key == "finalScore") {
                    hdr.finalScore = static_cast<std::uint32_t>(std::stoul(val));
                } else if (key == "date") {
                    hdr.date = val;
                }
            }
        } else {
            std::istringstream lineStream(line);
            std::uint32_t tick = 0;
            std::string dirStr;
            std::string modeStr;
            if (lineStream >> tick >> dirStr >> modeStr) {
                ReplayFrame frame {};
                frame.tick = tick;
                frame.cmd.direction = stringToDirection(dirStr);
                frame.cmd.drawMode = stringToDrawMode(modeStr);
                frames.push_back(frame);
            }
        }
    }

    outHeader = hdr;
    outFrames = std::move(frames);
    return true;
}

std::string ReplaySystem::serializeJson(const ReplayHeader& header, const std::vector<ReplayFrame>& frames) noexcept
{
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"version\": " << header.version << ",\n";
    oss << "  \"game\": {\n";
    oss << "    \"mode\": \"" << gameModeToString(header.mode) << "\",\n";
    oss << "    \"width\": " << header.playfieldWidth << ",\n";
    oss << "    \"height\": " << header.playfieldHeight << ",\n";
    oss << "    \"targetPercent\": " << header.targetPercent << ",\n";
    oss << "    \"baseDelayMs\": " << header.baseDelayMs << ",\n";
    oss << "    \"seed\": " << header.seed << "\n";
    oss << "  },\n";
    oss << "  \"metadata\": {\n";
    oss << "    \"totalTicks\": " << header.totalTicks << ",\n";
    oss << "    \"finalScore\": " << header.finalScore << ",\n";
    oss << "    \"date\": \"" << header.date << "\"\n";
    oss << "  },\n";
    oss << "  \"inputs\": [\n";

    for (std::size_t i = 0; i < frames.size(); ++i) {
        const auto& f = frames[i];
        oss << "    {\"tick\": " << f.tick << ", \"dir\": \"" << directionToString(f.cmd.direction)
            << "\", \"mode\": \"" << drawModeToString(f.cmd.drawMode) << "\"}";
        if (i + 1 < frames.size()) {
            oss << ",";
        }
        oss << "\n";
    }

    oss << "  ]\n";
    oss << "}\n";
    return oss.str();
}

namespace {
    [[nodiscard]] std::string extractJsonStringValue(const std::string& json, const std::string& key) noexcept
    {
        const std::string needle = "\"" + key + "\":";
        const auto pos = json.find(needle);
        if (pos == std::string::npos) {
            return "";
        }
        auto valStart = json.find('"', pos + needle.size());
        if (valStart == std::string::npos) {
            return "";
        }
        ++valStart;
        const auto valEnd = json.find('"', valStart);
        if (valEnd == std::string::npos) {
            return "";
        }
        return json.substr(valStart, valEnd - valStart);
    }

    [[nodiscard]] std::uint64_t extractJsonNumberValue(const std::string& json, const std::string& key) noexcept
    {
        const std::string needle = "\"" + key + "\":";
        const auto pos = json.find(needle);
        if (pos == std::string::npos) {
            return 0;
        }
        const auto start = json.find_first_of("0123456789", pos + needle.size());
        if (start == std::string::npos) {
            return 0;
        }
        const auto end = json.find_first_not_of("0123456789", start);
        return std::stoull(json.substr(start, end - start));
    }
} // namespace

bool ReplaySystem::deserializeJson(
    const std::string& json, ReplayHeader& outHeader, std::vector<ReplayFrame>& outFrames) noexcept
{
    if (json.empty() || json.find('{') == std::string::npos || json.find('}') == std::string::npos
        || json.find("\"version\"") == std::string::npos || json.find("\"inputs\"") == std::string::npos) {
        return false;
    }

    try {
        ReplayHeader hdr {};
        hdr.version = static_cast<std::uint32_t>(extractJsonNumberValue(json, "version"));
        hdr.mode = stringToGameMode(extractJsonStringValue(json, "mode"));
        hdr.playfieldWidth = static_cast<std::int32_t>(extractJsonNumberValue(json, "width"));
        hdr.playfieldHeight = static_cast<std::int32_t>(extractJsonNumberValue(json, "height"));
        hdr.targetPercent = static_cast<std::uint16_t>(extractJsonNumberValue(json, "targetPercent"));
        hdr.baseDelayMs = static_cast<std::uint32_t>(extractJsonNumberValue(json, "baseDelayMs"));
        hdr.seed = static_cast<std::uint32_t>(extractJsonNumberValue(json, "seed"));
        hdr.totalTicks = static_cast<std::uint32_t>(extractJsonNumberValue(json, "totalTicks"));
        hdr.finalScore = static_cast<std::uint32_t>(extractJsonNumberValue(json, "finalScore"));
        hdr.date = extractJsonStringValue(json, "date");

        // Fallbacks for default values if width/height not in JSON
        if (hdr.playfieldWidth <= 0) {
            hdr.playfieldWidth = 80;
        }
        if (hdr.playfieldHeight <= 0) {
            hdr.playfieldHeight = 60;
        }
        if (hdr.targetPercent == 0) {
            hdr.targetPercent = 75;
        }
        if (hdr.baseDelayMs == 0) {
            hdr.baseDelayMs = 75;
        }

        std::vector<ReplayFrame> frames {};
        const auto inputsPos = json.find("\"inputs\":");
        if (inputsPos != std::string::npos) {
            std::size_t curPos = inputsPos;
            while ((curPos = json.find('{', curPos + 1)) != std::string::npos) {
                const auto closePos = json.find('}', curPos);
                if (closePos == std::string::npos) {
                    break;
                }
                const std::string block = json.substr(curPos, closePos - curPos + 1);
                ReplayFrame f {};
                f.tick = static_cast<std::uint32_t>(extractJsonNumberValue(block, "tick"));
                f.cmd.direction = stringToDirection(extractJsonStringValue(block, "dir"));
                f.cmd.drawMode = stringToDrawMode(extractJsonStringValue(block, "mode"));
                frames.push_back(f);
                curPos = closePos;
            }
        }

        outHeader = hdr;
        outFrames = std::move(frames);
        return true;
    } catch (...) {
        return false;
    }
}

bool ReplaySystem::saveToFile(
    const std::string& filepath, const ReplayHeader& header, const std::vector<ReplayFrame>& frames) noexcept
{
    std::ofstream ofs(filepath);
    if (!ofs.is_open()) {
        return false;
    }

    if (filepath.size() >= 5 && filepath.substr(filepath.size() - 5) == ".json") {
        ofs << serializeJson(header, frames);
    } else {
        ofs << serializeCompact(header, frames);
    }

    return true;
}

bool ReplaySystem::loadFromFile(
    const std::string& filepath, ReplayHeader& outHeader, std::vector<ReplayFrame>& outFrames) noexcept
{
    std::ifstream ifs(filepath);
    if (!ifs.is_open()) {
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    if (content.empty()) {
        return false;
    }

    // Auto-detect JSON vs compact text
    const auto firstNonSpace = content.find_first_not_of(" \t\r\n");
    if (firstNonSpace != std::string::npos && content[firstNonSpace] == '{') {
        return deserializeJson(content, outHeader, outFrames);
    }
    return deserializeCompact(content, outHeader, outFrames);
}

// ================= ReplayRecorder =================

void ReplayRecorder::start(const ReplayHeader& header) noexcept
{
    m_header = header;
    m_frames.clear();
    m_recording = true;
}

void ReplayRecorder::recordTick(std::uint32_t tick, PlayerCommand cmd) noexcept
{
    if (!m_recording) {
        return;
    }

    // Only record frames with active inputs
    if (cmd.direction != Direction::None || cmd.drawMode != DrawMode::None) {
        m_frames.push_back(ReplayFrame {tick, cmd});
    }
}

void ReplayRecorder::finish(std::uint32_t finalScore, std::uint32_t totalTicks) noexcept
{
    if (!m_recording) {
        return;
    }
    m_header.finalScore = finalScore;
    m_header.totalTicks = totalTicks;
    m_recording = false;
}

bool ReplayRecorder::saveToFile(const std::string& filepath) const noexcept
{
    return ReplaySystem::saveToFile(filepath, m_header, m_frames);
}

bool ReplayRecorder::isRecording() const noexcept
{
    return m_recording;
}

const ReplayHeader& ReplayRecorder::getHeader() const noexcept
{
    return m_header;
}

const std::vector<ReplayFrame>& ReplayRecorder::getFrames() const noexcept
{
    return m_frames;
}

void ReplayRecorder::reset() noexcept
{
    m_header = ReplayHeader {};
    m_frames.clear();
    m_recording = false;
}

// ================= ReplayPlayer =================

bool ReplayPlayer::loadFromFile(const std::string& filepath) noexcept
{
    m_loaded = ReplaySystem::loadFromFile(filepath, m_header, m_frames);
    return m_loaded;
}

bool ReplayPlayer::loadFromJson(const std::string& json) noexcept
{
    m_loaded = ReplaySystem::deserializeJson(json, m_header, m_frames);
    return m_loaded;
}

bool ReplayPlayer::loadFromCompact(const std::string& text) noexcept
{
    m_loaded = ReplaySystem::deserializeCompact(text, m_header, m_frames);
    return m_loaded;
}

PlayerCommand ReplayPlayer::getCommandForTick(std::uint32_t tick) const noexcept
{
    if (!m_loaded || m_frames.empty()) {
        return PlayerCommand {};
    }

    const auto it = std::lower_bound(
        m_frames.begin(), m_frames.end(), tick, [](const ReplayFrame& f, std::uint32_t t) { return f.tick < t; });

    if (it != m_frames.end() && it->tick == tick) {
        return it->cmd;
    }

    return PlayerCommand {};
}

bool ReplayPlayer::isFinished(std::uint32_t currentTick) const noexcept
{
    if (!m_loaded) {
        return true;
    }
    if (m_header.totalTicks > 0 && currentTick >= m_header.totalTicks) {
        return true;
    }
    if (!m_frames.empty() && currentTick > m_frames.back().tick + 30) {
        return true;
    }
    return false;
}

bool ReplayPlayer::isLoaded() const noexcept
{
    return m_loaded;
}

const ReplayHeader& ReplayPlayer::getHeader() const noexcept
{
    return m_header;
}

std::uint32_t ReplayPlayer::getTotalTicks() const noexcept
{
    return m_header.totalTicks;
}

void ReplayPlayer::reset() noexcept
{
    m_header = ReplayHeader {};
    m_frames.clear();
    m_loaded = false;
}

} // namespace qix
