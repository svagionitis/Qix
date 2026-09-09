#include "SaveSystem.h"
#include "ReplaySystem.h"
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace qix {

std::string SaveSystem::getDefaultFilePath() noexcept
{
    const char* home = std::getenv("HOME");
    if (home != nullptr && *home != '\0') {
        return std::string {home} + "/.qix_saved_game.json";
    }

    const char* userProfile = std::getenv("USERPROFILE");
    if (userProfile != nullptr && *userProfile != '\0') {
        return std::string {userProfile} + "/.qix_saved_game.json";
    }

    return ".qix_saved_game.json";
}

std::string SaveSystem::expandPath(const std::string& path) noexcept
{
    if (path.empty()) {
        return getDefaultFilePath();
    }

    if (path == "~") {
        const char* home = std::getenv("HOME");
        if (home != nullptr && *home != '\0') {
            return std::string {home};
        }
        const char* userProfile = std::getenv("USERPROFILE");
        if (userProfile != nullptr && *userProfile != '\0') {
            return std::string {userProfile};
        }
        return ".";
    }

    if (path.rfind("~/", 0) == 0) {
        const char* home = std::getenv("HOME");
        if (home != nullptr && *home != '\0') {
            return std::string {home} + path.substr(1);
        }
        const char* userProfile = std::getenv("USERPROFILE");
        if (userProfile != nullptr && *userProfile != '\0') {
            return std::string {userProfile} + path.substr(1);
        }
        return path.substr(2);
    }

    return path;
}

const char* SaveSystem::gameStateToString(GameState state) noexcept
{
    switch (state) {
    case GameState::Ready:
        return "Ready";
    case GameState::Playing:
        return "Playing";
    case GameState::LevelComplete:
        return "LevelComplete";
    case GameState::GameOver:
        return "GameOver";
    case GameState::NameEntry:
        return "NameEntry";
    case GameState::HallOfFame:
        return "HallOfFame";
    case GameState::Attract:
        return "Attract";
    default:
        return "Ready";
    }
}

GameState SaveSystem::stringToGameState(const std::string& str) noexcept
{
    if (str == "Playing") {
        return GameState::Playing;
    }
    if (str == "LevelComplete") {
        return GameState::LevelComplete;
    }
    if (str == "GameOver") {
        return GameState::GameOver;
    }
    if (str == "NameEntry") {
        return GameState::NameEntry;
    }
    if (str == "HallOfFame") {
        return GameState::HallOfFame;
    }
    if (str == "Attract") {
        return GameState::Attract;
    }
    return GameState::Ready;
}

const char* SaveSystem::cellStateToChar(CellState state) noexcept
{
    switch (state) {
    case CellState::Empty:
        return "E";
    case CellState::Border:
        return "B";
    case CellState::ClaimedSlow:
        return "S";
    case CellState::ClaimedFast:
        return "F";
    case CellState::ActiveStix:
        return "A";
    default:
        return "E";
    }
}

CellState SaveSystem::charToCellState(char ch) noexcept
{
    switch (ch) {
    case 'B':
    case '1':
        return CellState::Border;
    case 'S':
    case '2':
        return CellState::ClaimedSlow;
    case 'F':
    case '3':
        return CellState::ClaimedFast;
    case 'A':
    case '4':
        return CellState::ActiveStix;
    case 'E':
    case '0':
    default:
        return CellState::Empty;
    }
}

std::string SaveSystem::encodeCellsRle(const std::vector<CellState>& cells) noexcept
{
    if (cells.empty()) {
        return "";
    }

    std::ostringstream oss;
    char currentCh = cellStateToChar(cells[0])[0];
    std::size_t currentCount = 1;

    for (std::size_t i = 1; i < cells.size(); ++i) {
        const char ch = cellStateToChar(cells[i])[0];
        if (ch == currentCh) {
            ++currentCount;
        } else {
            oss << currentCount << currentCh;
            currentCh = ch;
            currentCount = 1;
        }
    }
    oss << currentCount << currentCh;
    return oss.str();
}

bool SaveSystem::decodeCellsRle(
    const std::string& rle, std::size_t expectedCount, std::vector<CellState>& outCells) noexcept
{
    outCells.clear();
    outCells.reserve(expectedCount);

    std::size_t count = 0;
    for (std::size_t i = 0; i < rle.size(); ++i) {
        const char c = rle[i];
        if (c >= '0' && c <= '9') {
            count = count * 10 + static_cast<std::size_t>(c - '0');
        } else if (c == 'E' || c == 'B' || c == 'S' || c == 'F' || c == 'A') {
            const std::size_t runLength = (count == 0) ? 1 : count;
            const CellState state = charToCellState(c);
            outCells.insert(outCells.end(), runLength, state);
            count = 0;
        }
    }

    return outCells.size() == expectedCount;
}

std::string SaveSystem::serializeJson(const GameStateSnapshot& snapshot) noexcept
{
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"version\": " << snapshot.version << ",\n";
    oss << "  \"date\": \"" << (snapshot.date.empty() ? "2026-09-09T12:00:00Z" : snapshot.date) << "\",\n";
    oss << "  \"mode\": \"" << ReplaySystem::gameModeToString(snapshot.mode) << "\",\n";
    oss << "  \"state\": \"" << gameStateToString(snapshot.state) << "\",\n";
    oss << "  \"baseDelayMs\": " << snapshot.baseDelayMs << ",\n";
    oss << "  \"currentDelayMs\": " << snapshot.currentDelayMs << ",\n";
    oss << "  \"timeRemainingMs\": " << snapshot.timeRemainingMs << ",\n";
    oss << "  \"nextExtraLifeScore\": " << snapshot.nextExtraLifeScore << ",\n";

    // stats
    oss << "  \"stats\": {\n";
    oss << "    \"score\": " << snapshot.stats.score << ",\n";
    oss << "    \"highScore\": " << snapshot.stats.highScore << ",\n";
    oss << "    \"claimedCells\": " << snapshot.stats.claimedCells << ",\n";
    oss << "    \"totalEmptyCells\": " << snapshot.stats.totalEmptyCells << ",\n";
    oss << "    \"claimedPercent\": " << snapshot.stats.claimedPercent << ",\n";
    oss << "    \"targetPercent\": " << snapshot.stats.targetPercent << ",\n";
    oss << "    \"lives\": " << static_cast<std::uint32_t>(snapshot.stats.lives) << ",\n";
    oss << "    \"level\": " << static_cast<std::uint32_t>(snapshot.stats.level) << ",\n";
    oss << "    \"multiplier\": " << static_cast<std::uint32_t>(snapshot.stats.multiplier) << ",\n";
    oss << "    \"splitBonus\": " << (snapshot.stats.splitBonus ? "true" : "false") << ",\n";
    oss << "    \"thresholdBonus\": " << snapshot.stats.thresholdBonus << ",\n";
    oss << "    \"qixTrapped\": " << (snapshot.stats.qixTrapped ? "true" : "false") << ",\n";
    oss << "    \"spiralBonus\": " << (snapshot.stats.spiralBonus ? "true" : "false") << ",\n";
    oss << "    \"qixRemainingPercent\": " << snapshot.stats.qixRemainingPercent << ",\n";
    oss << "    \"trapBonus\": " << snapshot.stats.trapBonus << "\n";
    oss << "  },\n";

    // playfield
    oss << "  \"playfield\": {\n";
    oss << "    \"width\": " << snapshot.playfield.width << ",\n";
    oss << "    \"height\": " << snapshot.playfield.height << ",\n";
    oss << "    \"cells\": \"" << snapshot.playfield.cellsRle << "\"\n";
    oss << "  },\n";

    // marker
    oss << "  \"marker\": {\n";
    oss << "    \"x\": " << snapshot.marker.position.x << ",\n";
    oss << "    \"y\": " << snapshot.marker.position.y << ",\n";
    oss << "    \"drawMode\": \"" << ReplaySystem::drawModeToString(snapshot.marker.drawMode) << "\",\n";
    oss << "    \"lives\": " << static_cast<std::uint32_t>(snapshot.marker.lives) << ",\n";
    oss << "    \"trail\": [";
    for (std::size_t i = 0; i < snapshot.marker.trail.size(); ++i) {
        const auto& pt = snapshot.marker.trail[i];
        oss << "{\"x\":" << pt.x << ",\"y\":" << pt.y << "}";
        if (i + 1 < snapshot.marker.trail.size()) {
            oss << ",";
        }
    }
    oss << "]\n";
    oss << "  },\n";

    // qixes
    oss << "  \"qixes\": [\n";
    for (std::size_t i = 0; i < snapshot.qixes.size(); ++i) {
        const auto& q = snapshot.qixes[i];
        oss << "    {\n";
        oss << "      \"p1\": {\"x\":" << q.p1.x << ",\"y\":" << q.p1.y << "},\n";
        oss << "      \"p2\": {\"x\":" << q.p2.x << ",\"y\":" << q.p2.y << "},\n";
        oss << "      \"vx1\": " << q.vx1 << ",\n";
        oss << "      \"vy1\": " << q.vy1 << ",\n";
        oss << "      \"vx2\": " << q.vx2 << ",\n";
        oss << "      \"vy2\": " << q.vy2 << ",\n";
        oss << "      \"segments\": [";
        for (std::size_t j = 0; j < q.segments.size(); ++j) {
            const auto& seg = q.segments[j];
            oss << "{\"x1\":" << seg.start.x << ",\"y1\":" << seg.start.y << ",\"x2\":" << seg.end.x
                << ",\"y2\":" << seg.end.y << "}";
            if (j + 1 < q.segments.size()) {
                oss << ",";
            }
        }
        oss << "]\n";
        oss << "    }" << (i + 1 < snapshot.qixes.size() ? ",\n" : "\n");
    }
    oss << "  ],\n";

    // sparxList
    oss << "  \"sparx\": [\n";
    for (std::size_t i = 0; i < snapshot.sparxList.size(); ++i) {
        const auto& s = snapshot.sparxList[i];
        oss << "    {\"x\":" << s.position.x << ",\"y\":" << s.position.y
            << ",\"clockwise\":" << (s.clockwise ? "true" : "false")
            << ",\"isSuper\":" << (s.isSuper ? "true" : "false") << "}";
        if (i + 1 < snapshot.sparxList.size()) {
            oss << ",\n";
        } else {
            oss << "\n";
        }
    }
    oss << "  ],\n";

    // fuse
    oss << "  \"fuse\": {\n";
    oss << "    \"idleLimit\": " << snapshot.fuse.idleLimit << ",\n";
    oss << "    \"idleCounter\": " << snapshot.fuse.idleCounter << ",\n";
    oss << "    \"isBurning\": " << (snapshot.fuse.isBurning ? "true" : "false") << ",\n";
    oss << "    \"trailIndex\": " << snapshot.fuse.trailIndex << ",\n";
    oss << "    \"x\": " << snapshot.fuse.position.x << ",\n";
    oss << "    \"y\": " << snapshot.fuse.position.y << "\n";
    oss << "  }\n";

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

    [[nodiscard]] std::int64_t extractJsonIntValue(
        const std::string& json, const std::string& key, std::size_t startOffset = 0) noexcept
    {
        const std::string needle = "\"" + key + "\":";
        const auto pos = json.find(needle, startOffset);
        if (pos == std::string::npos) {
            return 0;
        }
        const auto start = json.find_first_of("-0123456789", pos + needle.size());
        if (start == std::string::npos) {
            return 0;
        }
        const auto end = json.find_first_not_of("-0123456789", start);
        try {
            return std::stoll(json.substr(start, end - start));
        } catch (...) {
            return 0;
        }
    }

    [[nodiscard]] bool extractJsonBoolValue(
        const std::string& json, const std::string& key, std::size_t startOffset = 0) noexcept
    {
        const std::string needle = "\"" + key + "\":";
        const auto pos = json.find(needle, startOffset);
        if (pos == std::string::npos) {
            return false;
        }
        const auto valStart = json.find_first_not_of(" \t\r\n", pos + needle.size());
        if (valStart == std::string::npos) {
            return false;
        }
        return json.compare(valStart, 4, "true") == 0;
    }

} // namespace

bool SaveSystem::deserializeJson(const std::string& json, GameStateSnapshot& outSnapshot) noexcept
{
    if (json.empty() || json.find("\"version\"") == std::string::npos
        || json.find("\"playfield\"") == std::string::npos) {
        return false;
    }

    outSnapshot.version = static_cast<std::uint32_t>(extractJsonIntValue(json, "version"));
    if (outSnapshot.version != kSaveVersion) {
        return false;
    }
    outSnapshot.date = extractJsonStringValue(json, "date");
    outSnapshot.mode = ReplaySystem::stringToGameMode(extractJsonStringValue(json, "mode"));
    outSnapshot.state = stringToGameState(extractJsonStringValue(json, "state"));
    outSnapshot.baseDelayMs = static_cast<std::uint32_t>(extractJsonIntValue(json, "baseDelayMs"));
    outSnapshot.currentDelayMs = static_cast<std::uint32_t>(extractJsonIntValue(json, "currentDelayMs"));
    outSnapshot.timeRemainingMs = static_cast<std::uint32_t>(extractJsonIntValue(json, "timeRemainingMs"));
    outSnapshot.nextExtraLifeScore = static_cast<std::uint32_t>(extractJsonIntValue(json, "nextExtraLifeScore"));

    // stats
    const auto statsPos = json.find("\"stats\":");
    if (statsPos != std::string::npos) {
        outSnapshot.stats.score = static_cast<std::uint32_t>(extractJsonIntValue(json, "score", statsPos));
        outSnapshot.stats.highScore = static_cast<std::uint32_t>(extractJsonIntValue(json, "highScore", statsPos));
        outSnapshot.stats.claimedCells
            = static_cast<std::uint32_t>(extractJsonIntValue(json, "claimedCells", statsPos));
        outSnapshot.stats.totalEmptyCells
            = static_cast<std::uint32_t>(extractJsonIntValue(json, "totalEmptyCells", statsPos));
        outSnapshot.stats.claimedPercent
            = static_cast<std::uint16_t>(extractJsonIntValue(json, "claimedPercent", statsPos));
        outSnapshot.stats.targetPercent
            = static_cast<std::uint16_t>(extractJsonIntValue(json, "targetPercent", statsPos));
        outSnapshot.stats.lives = static_cast<std::uint8_t>(extractJsonIntValue(json, "lives", statsPos));
        outSnapshot.stats.level = static_cast<std::uint8_t>(extractJsonIntValue(json, "level", statsPos));
        outSnapshot.stats.multiplier = static_cast<std::uint8_t>(extractJsonIntValue(json, "multiplier", statsPos));
        outSnapshot.stats.splitBonus = extractJsonBoolValue(json, "splitBonus", statsPos);
        outSnapshot.stats.thresholdBonus
            = static_cast<std::uint32_t>(extractJsonIntValue(json, "thresholdBonus", statsPos));
        outSnapshot.stats.qixTrapped = extractJsonBoolValue(json, "qixTrapped", statsPos);
        outSnapshot.stats.spiralBonus = extractJsonBoolValue(json, "spiralBonus", statsPos);
        outSnapshot.stats.qixRemainingPercent
            = static_cast<std::uint16_t>(extractJsonIntValue(json, "qixRemainingPercent", statsPos));
        outSnapshot.stats.trapBonus = static_cast<std::uint32_t>(extractJsonIntValue(json, "trapBonus", statsPos));
        outSnapshot.stats.timeRemainingMs = outSnapshot.timeRemainingMs;
        outSnapshot.stats.currentDelayMs = outSnapshot.currentDelayMs;
        outSnapshot.stats.mode = outSnapshot.mode;
    }

    // playfield
    const auto pfPos = json.find("\"playfield\":");
    if (pfPos != std::string::npos) {
        outSnapshot.playfield.width = static_cast<std::int32_t>(extractJsonIntValue(json, "width", pfPos));
        outSnapshot.playfield.height = static_cast<std::int32_t>(extractJsonIntValue(json, "height", pfPos));
        const auto cellsNeedle = std::string {"\"cells\":"};
        const auto cellsPos = json.find(cellsNeedle, pfPos);
        if (cellsPos != std::string::npos) {
            auto vStart = json.find('"', cellsPos + cellsNeedle.size());
            if (vStart != std::string::npos) {
                ++vStart;
                const auto vEnd = json.find('"', vStart);
                if (vEnd != std::string::npos) {
                    outSnapshot.playfield.cellsRle = json.substr(vStart, vEnd - vStart);
                }
            }
        }
    }

    // marker
    const auto markerPos = json.find("\"marker\":");
    if (markerPos != std::string::npos) {
        outSnapshot.marker.position.x = static_cast<std::int32_t>(extractJsonIntValue(json, "x", markerPos));
        outSnapshot.marker.position.y = static_cast<std::int32_t>(extractJsonIntValue(json, "y", markerPos));
        outSnapshot.marker.drawMode = ReplaySystem::stringToDrawMode(extractJsonStringValue(json, "drawMode"));
        outSnapshot.marker.lives = static_cast<std::uint8_t>(extractJsonIntValue(json, "lives", markerPos));

        // trail
        outSnapshot.marker.trail.clear();
        const auto trailStart = json.find("\"trail\":", markerPos);
        if (trailStart != std::string::npos) {
            const auto arrStart = json.find('[', trailStart);
            const auto arrEnd = json.find(']', arrStart);
            if (arrStart != std::string::npos && arrEnd != std::string::npos) {
                std::size_t cur = arrStart + 1;
                while (cur < arrEnd) {
                    const auto objStart = json.find('{', cur);
                    if (objStart == std::string::npos || objStart >= arrEnd) {
                        break;
                    }
                    const auto objEnd = json.find('}', objStart);
                    if (objEnd == std::string::npos || objEnd > arrEnd) {
                        break;
                    }
                    const std::string objStr = json.substr(objStart, objEnd - objStart + 1);
                    const auto x = static_cast<std::int32_t>(extractJsonIntValue(objStr, "x"));
                    const auto y = static_cast<std::int32_t>(extractJsonIntValue(objStr, "y"));
                    outSnapshot.marker.trail.push_back(Point {x, y});
                    cur = objEnd + 1;
                }
            }
        }
    }

    // qixes
    outSnapshot.qixes.clear();
    const auto qixArrPos = json.find("\"qixes\":");
    if (qixArrPos != std::string::npos) {
        const auto arrStart = json.find('[', qixArrPos);
        if (arrStart != std::string::npos) {
            int bracketDepth = 0;
            std::size_t arrEnd = std::string::npos;
            for (std::size_t b = arrStart; b < json.size(); ++b) {
                if (json[b] == '[') {
                    ++bracketDepth;
                } else if (json[b] == ']') {
                    --bracketDepth;
                    if (bracketDepth == 0) {
                        arrEnd = b;
                        break;
                    }
                }
            }
            if (arrEnd != std::string::npos) {
                std::size_t cur = arrStart + 1;
                while (cur < arrEnd) {
                    const auto objStart = json.find('{', cur);
                    if (objStart == std::string::npos || objStart >= arrEnd) {
                        break;
                    }
                    // find corresponding closing brace taking nested segments into account
                    int braceDepth = 0;
                    std::size_t objEnd = objStart;
                    for (; objEnd <= arrEnd; ++objEnd) {
                        if (json[objEnd] == '{') {
                            ++braceDepth;
                        } else if (json[objEnd] == '}') {
                            --braceDepth;
                            if (braceDepth == 0) {
                                break;
                            }
                        }
                    }
                    if (braceDepth != 0 || objEnd >= arrEnd) {
                        break;
                    }

                    const std::string qixJson = json.substr(objStart, objEnd - objStart + 1);
                    QixSnapshot q {};
                    const auto p1Pos = qixJson.find("\"p1\":");
                    if (p1Pos != std::string::npos) {
                        q.p1.x = static_cast<std::int32_t>(extractJsonIntValue(qixJson, "x", p1Pos));
                        q.p1.y = static_cast<std::int32_t>(extractJsonIntValue(qixJson, "y", p1Pos));
                    }
                    const auto p2Pos = qixJson.find("\"p2\":");
                    if (p2Pos != std::string::npos) {
                        q.p2.x = static_cast<std::int32_t>(extractJsonIntValue(qixJson, "x", p2Pos));
                        q.p2.y = static_cast<std::int32_t>(extractJsonIntValue(qixJson, "y", p2Pos));
                    }
                    q.vx1 = static_cast<std::int32_t>(extractJsonIntValue(qixJson, "vx1"));
                    q.vy1 = static_cast<std::int32_t>(extractJsonIntValue(qixJson, "vy1"));
                    q.vx2 = static_cast<std::int32_t>(extractJsonIntValue(qixJson, "vx2"));
                    q.vy2 = static_cast<std::int32_t>(extractJsonIntValue(qixJson, "vy2"));

                    // segments
                    const auto segPos = qixJson.find("\"segments\":");
                    if (segPos != std::string::npos) {
                        const auto sStart = qixJson.find('[', segPos);
                        const auto sEnd = qixJson.find(']', sStart);
                        if (sStart != std::string::npos && sEnd != std::string::npos) {
                            std::size_t sCur = sStart + 1;
                            while (sCur < sEnd) {
                                const auto soStart = qixJson.find('{', sCur);
                                if (soStart == std::string::npos || soStart >= sEnd) {
                                    break;
                                }
                                const auto soEnd = qixJson.find('}', soStart);
                                if (soEnd == std::string::npos || soEnd > sEnd) {
                                    break;
                                }
                                const std::string sObj = qixJson.substr(soStart, soEnd - soStart + 1);
                                LineSegment seg {};
                                seg.start.x = static_cast<std::int32_t>(extractJsonIntValue(sObj, "x1"));
                                seg.start.y = static_cast<std::int32_t>(extractJsonIntValue(sObj, "y1"));
                                seg.end.x = static_cast<std::int32_t>(extractJsonIntValue(sObj, "x2"));
                                seg.end.y = static_cast<std::int32_t>(extractJsonIntValue(sObj, "y2"));
                                q.segments.push_back(seg);
                                sCur = soEnd + 1;
                            }
                        }
                    }
                    outSnapshot.qixes.push_back(std::move(q));
                    cur = objEnd + 1;
                }
            }
        }
    }

    // sparxList
    outSnapshot.sparxList.clear();
    const auto sparxArrPos = json.find("\"sparx\":");
    if (sparxArrPos != std::string::npos) {
        const auto arrStart = json.find('[', sparxArrPos);
        const auto arrEnd = json.find(']', arrStart);
        if (arrStart != std::string::npos && arrEnd != std::string::npos) {
            std::size_t cur = arrStart + 1;
            while (cur < arrEnd) {
                const auto objStart = json.find('{', cur);
                if (objStart == std::string::npos || objStart >= arrEnd) {
                    break;
                }
                const auto objEnd = json.find('}', objStart);
                if (objEnd == std::string::npos || objEnd > arrEnd) {
                    break;
                }
                const std::string sObj = json.substr(objStart, objEnd - objStart + 1);
                SparxSnapshot s {};
                s.position.x = static_cast<std::int32_t>(extractJsonIntValue(sObj, "x"));
                s.position.y = static_cast<std::int32_t>(extractJsonIntValue(sObj, "y"));
                s.clockwise = extractJsonBoolValue(sObj, "clockwise");
                s.isSuper = extractJsonBoolValue(sObj, "isSuper");
                outSnapshot.sparxList.push_back(s);
                cur = objEnd + 1;
            }
        }
    }

    // fuse
    const auto fusePos = json.find("\"fuse\":");
    if (fusePos != std::string::npos) {
        outSnapshot.fuse.idleLimit = static_cast<std::uint32_t>(extractJsonIntValue(json, "idleLimit", fusePos));
        outSnapshot.fuse.idleCounter = static_cast<std::uint32_t>(extractJsonIntValue(json, "idleCounter", fusePos));
        outSnapshot.fuse.isBurning = extractJsonBoolValue(json, "isBurning", fusePos);
        outSnapshot.fuse.trailIndex = static_cast<std::size_t>(extractJsonIntValue(json, "trailIndex", fusePos));
        outSnapshot.fuse.position.x = static_cast<std::int32_t>(extractJsonIntValue(json, "x", fusePos));
        outSnapshot.fuse.position.y = static_cast<std::int32_t>(extractJsonIntValue(json, "y", fusePos));
    }

    return true;
}

bool SaveSystem::saveToFile(const std::string& filepath, const GameStateSnapshot& snapshot) noexcept
{
    const std::string actualPath = expandPath(filepath);
    std::ofstream ofs(actualPath);
    if (!ofs.is_open()) {
        return false;
    }

    const std::string json = serializeJson(snapshot);
    ofs << json;
    return ofs.good();
}

bool SaveSystem::loadFromFile(const std::string& filepath, GameStateSnapshot& outSnapshot) noexcept
{
    const std::string actualPath = expandPath(filepath);
    std::ifstream ifs(actualPath);
    if (!ifs.is_open()) {
        return false;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    return deserializeJson(oss.str(), outSnapshot);
}

} // namespace qix
