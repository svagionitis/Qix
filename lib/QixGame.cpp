#include "QixGame.h"
#include "SaveSystem.h"

namespace qix {

QixGame::QixGame(std::int32_t width, std::int32_t height, std::uint16_t targetPercent, GameMode mode,
    std::uint32_t baseDelayMs, std::optional<bool> randomSpawns) noexcept
    : m_playfield {width, height}
    , m_marker {Point {width / 2, height - 1}, 3, mode}
    , m_fuse {25}
    , m_fill {width, height}
    , m_mode {mode}
    , m_baseDelayMs {SpeedConfig::clampDelay(baseDelayMs)}
    , m_currentDelayMs {m_baseDelayMs}
    , m_randomSpawns {randomSpawns.value_or(mode == GameMode::Modern)}
{
    m_stats.targetPercent = targetPercent;
    m_stats.totalEmptyCells = m_playfield.getInteriorCount();
    m_stats.mode = mode;
    m_stats.currentDelayMs = m_currentDelayMs;
    static_cast<void>(m_highScoreTable.loadFromFile(HighScoreTable::getDefaultFilePath()));
    m_stats.highScore = m_highScoreTable.getHighScore();
    reset();
}

void QixGame::step(std::uint32_t deltaMs) noexcept
{
    m_events.clear();
    if (m_isPaused) {
        updateSnapshot();
        return;
    }
    if (m_state == GameState::GameOver || m_state == GameState::LevelComplete || m_state == GameState::NameEntry
        || m_state == GameState::HallOfFame) {
        if (m_state == GameState::GameOver || m_state == GameState::HallOfFame) {
            m_idleTimerMs += deltaMs;
            if (m_idleTimerMs >= IdleTimeoutMs) {
                startAttractMode();
                return;
            }
        }
        updateSnapshot();
        return;
    }

    if (m_state == GameState::Attract) {
        updateAttractCycle(deltaMs);
        return;
    }

    if (m_state == GameState::Ready) {
        if (m_pendingCmd.direction != Direction::None || m_pendingCmd.drawMode != DrawMode::None) {
            m_state = GameState::Playing;
            m_idleTimerMs = 0;
        } else {
            m_idleTimerMs += deltaMs;
            if (m_idleTimerMs >= IdleTimeoutMs) {
                startAttractMode();
                return;
            }
            updateSnapshot();
            return;
        }
    }

    updateLevelTimer(deltaMs);

    const bool wasDrawing = m_marker.isDrawing();
    const auto oldTrailSize = m_marker.getTrail().size();

    // 1. Move Player Marker
    const bool moved = m_marker.move(m_playfield, m_pendingCmd);

    // 2. Loop Closure Check: Marker was drawing and returned to a border/claimed cell
    if (wasDrawing && moved && m_marker.getTrail().size() > oldTrailSize) {
        const auto currentPos = m_marker.getPosition();
        const auto cellState = m_playfield.getCell(currentPos.x, currentPos.y);

        const bool isClosingCell = (m_mode == GameMode::Classic)
            ? (cellState == CellState::Border)
            : (cellState == CellState::Border || cellState == CellState::ClaimedSlow
                || cellState == CellState::ClaimedFast);

        // If the marker reached border or claimed territory after stepping into empty space
        if (isClosingCell) {
            std::vector<Point> qixPositions {};
            for (const auto& qix : m_qixList) {
                qixPositions.push_back(qix.getHead().start);
            }

            const auto completedTrail = m_marker.getTrail();
            const auto drawMode = m_marker.getDrawMode();
            const auto fillRes = m_fill.execute(
                m_playfield, completedTrail, qixPositions, drawMode, m_stats.targetPercent, m_stats.multiplier);

            if (fillRes.claimedCellsCount > 0) {
                m_events.push_back(GameEvent {GameEventType::TerritoryCapture, currentPos, completedTrail, drawMode});
            }

            m_stats.score += fillRes.pointsAwarded;
            m_stats.claimedCells = fillRes.totalClaimedSoFar;
            m_stats.claimedPercent = fillRes.claimedPercent;
            m_stats.thresholdBonus = fillRes.thresholdBonus;
            m_stats.qixTrapped = fillRes.qixTrapped;
            m_stats.spiralBonus = fillRes.spiralBonus;
            m_stats.qixRemainingPercent = fillRes.qixRemainingPercent;
            m_stats.trapBonus = fillRes.trapBonus;

            while (m_stats.score >= m_nextExtraLifeScore) {
                m_marker.incrementLives();
                m_nextExtraLifeScore += ExtraLifeInterval;
            }
            m_stats.nextExtraLifeScore = m_nextExtraLifeScore;
            m_stats.lives = m_marker.getLives();

            m_marker.clearTrail();
            m_fuse.reset();

            if (fillRes.splitOccurred) {
                if (m_stats.multiplier < 9) {
                    ++m_stats.multiplier;
                }
                m_stats.splitBonus = true;
                m_state = GameState::LevelComplete;
            } else if (fillRes.thresholdMet) {
                m_stats.splitBonus = false;
                m_state = GameState::LevelComplete;
            }
        }
    }

    // 3. Update Qix entities
    for (auto& qix : m_qixList) {
        qix.update(m_playfield);
    }

    // 4. Update Sparx entities
    for (auto& sparx : m_sparxList) {
        sparx.update(m_playfield);
    }

    // 5. Update Fuse
    const bool markerActive = moved || m_marker.isPacingWait();
    m_fuse.update(m_marker.isDrawing(), markerActive, m_marker.getTrail());

    // 6. Audit Collisions
    const auto collision = CollisionDetector::check(m_marker, m_qixList, m_sparxList, m_fuse);
    if (collision != CollisionEvent::None) {
        m_events.push_back(GameEvent {GameEventType::MarkerDeath, m_marker.getPosition(), {}, m_marker.getDrawMode()});
        handleDeath();
    }

    // 7. Refresh View Snapshot
    updateSnapshot();

    // Consume single-tick input command
    m_pendingCmd.direction = Direction::None;
}

void QixGame::handleInput(PlayerCommand cmd) noexcept
{
    if (m_isPaused) {
        return;
    }

    if (m_state == GameState::Attract) {
        if (cmd.direction != Direction::None || cmd.drawMode != DrawMode::None) {
            exitAttractMode();
            return;
        }
    } else {
        if (cmd.direction != Direction::None || cmd.drawMode != DrawMode::None) {
            m_idleTimerMs = 0;
        }
    }

    m_pendingCmd = cmd;
    if (m_state == GameState::NameEntry) {
        handleNameEntryInput(cmd);
    }
}

const GameView& QixGame::getView() const noexcept
{
    return m_view;
}

void QixGame::reset() noexcept
{
    m_isPaused = false;
    m_idleTimerMs = 0;
    m_stats.isAttractMode = false;
    m_playfield.initBorders();
    m_marker = Marker {Point {m_playfield.getWidth() / 2, m_playfield.getHeight() - 1}, 3};
    m_stats.score = 0;
    m_stats.claimedCells = 0;
    m_stats.claimedPercent = 0;
    m_stats.lives = 3;
    m_nextExtraLifeScore = ExtraLifeInterval;
    m_stats.nextExtraLifeScore = m_nextExtraLifeScore;
    m_stats.level = 1;
    m_stats.multiplier = 1;
    m_stats.splitBonus = false;
    m_stats.thresholdBonus = 0;
    m_stats.qixTrapped = false;
    m_stats.spiralBonus = false;
    m_stats.qixRemainingPercent = 0;
    m_stats.trapBonus = 0;
    m_timeRemainingMs = computeLevelTimeMs(1);
    m_stats.totalLevelTimeMs = m_timeRemainingMs;
    m_stats.timeRemainingMs = m_timeRemainingMs;
    m_stats.timeUp = false;
    m_currentDelayMs = m_baseDelayMs;
    m_stats.currentDelayMs = m_currentDelayMs;
    m_stats.highScore = m_highScoreTable.getHighScore();
    m_state = GameState::Ready;

    setupEntities();
    updateSnapshot();
}

void QixGame::nextLevel() noexcept
{
    m_isPaused = false;
    m_playfield.initBorders();
    m_marker.resetPosition(Point {m_playfield.getWidth() / 2, m_playfield.getHeight() - 1});
    m_stats.claimedCells = 0;
    m_stats.claimedPercent = 0;
    m_stats.splitBonus = false;
    m_stats.thresholdBonus = 0;
    m_stats.qixTrapped = false;
    m_stats.spiralBonus = false;
    m_stats.qixRemainingPercent = 0;
    m_stats.trapBonus = 0;
    ++m_stats.level;
    m_timeRemainingMs = computeLevelTimeMs(m_stats.level);
    m_stats.totalLevelTimeMs = m_timeRemainingMs;
    m_stats.timeRemainingMs = m_timeRemainingMs;
    m_stats.timeUp = false;
    m_currentDelayMs = SpeedConfig::computeLevelDelay(m_baseDelayMs, m_stats.level);
    m_stats.currentDelayMs = m_currentDelayMs;
    m_state = GameState::Ready;

    setupEntities();
    updateSnapshot();
}

void QixGame::setupEntities() noexcept
{
    m_qixList.clear();

    const auto pw = m_playfield.getWidth();
    const auto ph = m_playfield.getHeight();

    if (!m_randomSpawns) {
        // Spawn Qix near center of empty field
        const auto cx = pw / 2;
        const auto cy = ph / 2;
        LineSegment qixLine {Point {cx - 5, cy}, Point {cx + 5, cy}};
        m_qixList.emplace_back(qixLine, 10);

        // If level 2 or higher, add a second Qix
        if (m_stats.level >= 2) {
            LineSegment qixLine2 {Point {cx, cy - 5}, Point {cx, cy + 5}};
            m_qixList.emplace_back(qixLine2, 10);
        }

        m_sparxList.clear();
        const bool isSuper = (m_stats.level >= 3);
        // Sparx 1: Clockwise from top-left
        m_sparxList.emplace_back(Point {1, 0}, true, m_mode, isSuper);
        // Sparx 2: Counter-clockwise from top-right
        m_sparxList.emplace_back(Point {pw - 2, 0}, false, m_mode, isSuper);
    } else {
        // Modern / Randomized Spawns:
        // 1. Qix spawn: within safe interior margins
        const std::int32_t minX = std::max<std::int32_t>(10, pw / 6);
        const std::int32_t maxX = std::max<std::int32_t>(minX + 5, pw - minX);
        const std::int32_t minY = std::max<std::int32_t>(8, ph / 6);
        const std::int32_t maxY = std::max<std::int32_t>(minY + 5, ph - 16);

        std::uniform_int_distribution<std::int32_t> distQX(minX, maxX);
        std::uniform_int_distribution<std::int32_t> distQY(minY, maxY);
        std::uniform_int_distribution<int> distBool(0, 1);

        const auto q1x = distQX(m_spawnRng);
        const auto q1y = distQY(m_spawnRng);
        const bool q1Vert = (distBool(m_spawnRng) != 0);

        LineSegment qixLine = q1Vert ? LineSegment {Point {q1x, std::max<std::int32_t>(1, q1y - 5)},
                                  Point {q1x, std::min<std::int32_t>(ph - 2, q1y + 5)}}
                                     : LineSegment {Point {std::max<std::int32_t>(1, q1x - 5), q1y},
                                         Point {std::min<std::int32_t>(pw - 2, q1x + 5), q1y}};
        m_qixList.emplace_back(qixLine, 10);

        if (m_stats.level >= 2) {
            std::int32_t q2x = distQX(m_spawnRng);
            std::int32_t q2y = distQY(m_spawnRng);
            for (int attempts = 0; attempts < 10; ++attempts) {
                if (std::abs(q2x - q1x) + std::abs(q2y - q1y) >= 15) {
                    break;
                }
                q2x = distQX(m_spawnRng);
                q2y = distQY(m_spawnRng);
            }
            const bool q2Vert = !q1Vert;
            LineSegment qixLine2 = q2Vert ? LineSegment {Point {q2x, std::max<std::int32_t>(1, q2y - 5)},
                                       Point {q2x, std::min<std::int32_t>(ph - 2, q2y + 5)}}
                                          : LineSegment {Point {std::max<std::int32_t>(1, q2x - 5), q2y},
                                              Point {std::min<std::int32_t>(pw - 2, q2x + 5), q2y}};
            m_qixList.emplace_back(qixLine2, 10);
        }

        // 2. Sparx spawn: scan all border cells, enforcing safe distance from marker
        m_sparxList.clear();
        const bool isSuper = (m_stats.level >= 3);
        const auto markerPos = m_marker.getPosition();

        std::vector<Point> safeBorderCells;
        safeBorderCells.reserve(static_cast<std::size_t>(pw * 2 + ph * 2));

        for (std::int32_t y = 0; y < ph; ++y) {
            for (std::int32_t x = 0; x < pw; ++x) {
                if (m_playfield.getCell(x, y) == CellState::Border) {
                    const auto distToMarker = std::abs(x - markerPos.x) + std::abs(y - markerPos.y);
                    if (distToMarker >= 25) {
                        safeBorderCells.emplace_back(Point {x, y});
                    }
                }
            }
        }

        if (safeBorderCells.empty()) {
            m_sparxList.emplace_back(Point {1, 0}, true, m_mode, isSuper);
            m_sparxList.emplace_back(Point {pw - 2, 0}, false, m_mode, isSuper);
        } else {
            std::uniform_int_distribution<std::size_t> distIdx(0, safeBorderCells.size() - 1);
            const auto idx1 = distIdx(m_spawnRng);
            const auto pos1 = safeBorderCells[idx1];
            const bool cw1 = (distBool(m_spawnRng) != 0);
            m_sparxList.emplace_back(pos1, cw1, m_mode, isSuper);

            std::size_t idx2 = distIdx(m_spawnRng);
            for (int attempts = 0; attempts < 10; ++attempts) {
                const auto candidate = safeBorderCells[idx2];
                if (std::abs(candidate.x - pos1.x) + std::abs(candidate.y - pos1.y) >= 15) {
                    break;
                }
                idx2 = distIdx(m_spawnRng);
            }
            const auto pos2 = safeBorderCells[idx2];
            const bool cw2 = (distBool(m_spawnRng) != 0);
            m_sparxList.emplace_back(pos2, cw2, m_mode, isSuper);
        }
    }

    m_fuse.reset();
    m_fuse.setIdleLimit(computeFuseLimit(m_stats.level));
}

void QixGame::updateSnapshot() noexcept
{
    m_view.playfield = &m_playfield;
    m_view.markerPos = m_marker.getPosition();
    m_view.drawMode = m_marker.getDrawMode();
    m_view.stixTrail = m_marker.getTrail();

    m_view.qixRibbons.clear();
    for (const auto& qix : m_qixList) {
        m_view.qixRibbons.push_back(qix.getSegments());
    }

    m_view.sparxPositions.clear();
    m_view.sparxList.clear();
    for (const auto& sparx : m_sparxList) {
        m_view.sparxPositions.push_back(sparx.getPosition());
        m_view.sparxList.push_back(SparxInfo {sparx.getPosition(), sparx.isSuper()});
    }

    m_view.fusePos = m_fuse.getPosition();
    m_stats.lives = m_marker.getLives();
    m_stats.mode = m_mode;
    if (m_stats.score > m_stats.highScore && m_state != GameState::Attract) {
        m_stats.highScore = m_stats.score;
    }
    m_stats.isAttractMode = (m_state == GameState::Attract);
    m_stats.attractStage = m_attractStage;
    m_stats.attractTimerMs = m_attractStageTimerMs;
    m_view.stats = m_stats;
    m_view.state = m_state;
    m_view.mode = m_mode;
    m_view.nameEntry = m_nameEntry;
    m_view.highScoreTable = &m_highScoreTable;
    m_view.isAttractMode = (m_state == GameState::Attract);
    m_view.attractStage = m_attractStage;
    m_view.isPaused = m_isPaused;
    m_view.events = m_events;
}

void QixGame::handleDeath() noexcept
{
    m_isPaused = false;
    m_marker.decrementLives();
    clearActiveStix();
    m_fuse.reset();

    if (!m_marker.isAlive()) {
        if (m_highScoreTable.qualifies(m_stats.score)) {
            m_state = GameState::NameEntry;
            m_nameEntry.initials = {'A', 'A', 'A'};
            m_nameEntry.cursorIndex = 0;
            m_nameEntry.rank = m_highScoreTable.getRank(m_stats.score);
        } else {
            m_state = GameState::GameOver;
        }
    } else {
        // Respawn marker at bottom safe border
        m_marker.resetPosition(Point {m_playfield.getWidth() / 2, m_playfield.getHeight() - 1});
        setupEntities();
        m_timeRemainingMs = computeLevelTimeMs(m_stats.level);
        m_stats.totalLevelTimeMs = m_timeRemainingMs;
        m_stats.timeRemainingMs = m_timeRemainingMs;
        m_stats.timeUp = false;
        m_state = GameState::Ready;
    }

    updateSnapshot();
}

void QixGame::clearActiveStix() noexcept
{
    const auto& trail = m_marker.getTrail();
    for (const auto& pt : trail) {
        if (m_playfield.getCell(pt.x, pt.y) == CellState::ActiveStix) {
            m_playfield.setCell(pt.x, pt.y, CellState::Empty);
        }
    }
    m_marker.clearTrail();
}

GameMode QixGame::getGameMode() const noexcept
{
    return m_mode;
}

std::uint32_t QixGame::getBaseDelayMs() const noexcept
{
    return m_baseDelayMs;
}

std::uint32_t QixGame::getCurrentDelayMs() const noexcept
{
    return m_currentDelayMs;
}

void QixGame::setBaseDelayMs(std::uint32_t delayMs) noexcept
{
    m_baseDelayMs = SpeedConfig::clampDelay(delayMs);
    m_currentDelayMs = SpeedConfig::computeLevelDelay(m_baseDelayMs, m_stats.level);
    m_stats.currentDelayMs = m_currentDelayMs;
    updateSnapshot();
}

void QixGame::updateLevelTimer(std::uint32_t deltaMs) noexcept
{
    if (m_state != GameState::Playing) {
        return;
    }

    if (deltaMs >= m_timeRemainingMs) {
        m_stats.timeUp = true;
        m_timeRemainingMs = 15000U;
        spawnEscalationSparx();
    } else {
        m_timeRemainingMs -= deltaMs;
    }
    m_stats.timeRemainingMs = m_timeRemainingMs;
}

void QixGame::spawnEscalationSparx() noexcept
{
    if (m_sparxList.size() >= 8) {
        return;
    }

    const bool clockwise = (m_sparxList.size() % 2 == 0);
    const auto spawnX = clockwise ? 1 : (m_playfield.getWidth() - 2);
    m_sparxList.emplace_back(Point {spawnX, 0}, clockwise, m_mode, true);
}

const HighScoreTable& QixGame::getHighScoreTable() const noexcept
{
    return m_highScoreTable;
}

void QixGame::inputInitialsChar(char c) noexcept
{
    if (m_state != GameState::NameEntry) {
        return;
    }

    const auto uc = static_cast<unsigned char>(c);
    if (std::isalnum(uc) == 0 && c != '!' && c != '.' && c != '?') {
        return;
    }

    m_nameEntry.initials[m_nameEntry.cursorIndex] = static_cast<char>(std::toupper(uc));
    if (m_nameEntry.cursorIndex < 2) {
        ++m_nameEntry.cursorIndex;
    }
    updateSnapshot();
}

void QixGame::confirmInitials() noexcept
{
    if (m_state != GameState::NameEntry) {
        return;
    }

    const std::string initialsStr(m_nameEntry.initials.data(), 3);
    static_cast<void>(m_highScoreTable.insert(initialsStr, m_stats.score, m_stats.level, m_mode));
    static_cast<void>(m_highScoreTable.saveToFile(HighScoreTable::getDefaultFilePath()));
    m_stats.highScore = m_highScoreTable.getHighScore();
    m_state = GameState::HallOfFame;
    updateSnapshot();
}

void QixGame::handleNameEntryInput(PlayerCommand cmd) noexcept
{
    if (m_state != GameState::NameEntry) {
        return;
    }

    if (cmd.direction == Direction::Up) {
        char& c = m_nameEntry.initials[m_nameEntry.cursorIndex];
        if (c == 'Z') {
            c = 'A';
        } else if (c >= 'A' && c < 'Z') {
            ++c;
        } else {
            c = 'A';
        }
    } else if (cmd.direction == Direction::Down) {
        char& c = m_nameEntry.initials[m_nameEntry.cursorIndex];
        if (c == 'A') {
            c = 'Z';
        } else if (c > 'A' && c <= 'Z') {
            --c;
        } else {
            c = 'Z';
        }
    } else if (cmd.direction == Direction::Left) {
        if (m_nameEntry.cursorIndex > 0) {
            --m_nameEntry.cursorIndex;
        }
    } else if (cmd.direction == Direction::Right) {
        if (m_nameEntry.cursorIndex < 2) {
            ++m_nameEntry.cursorIndex;
        }
    } else if (cmd.drawMode != DrawMode::None) {
        if (m_nameEntry.cursorIndex < 2) {
            ++m_nameEntry.cursorIndex;
        } else {
            confirmInitials();
            return;
        }
    }

    updateSnapshot();
}

void QixGame::startAttractMode() noexcept
{
    m_isPaused = false;
    m_state = GameState::Attract;
    m_attractStage = AttractStage::TitleScores;
    m_attractStageTimerMs = 0;
    m_idleTimerMs = 0;
    resetDemoPlayfield();
    updateSnapshot();
}

void QixGame::exitAttractMode() noexcept
{
    m_state = GameState::Ready;
    m_idleTimerMs = 0;
    reset();
}

bool QixGame::isAttractMode() const noexcept
{
    return m_state == GameState::Attract;
}

void QixGame::setDemoDurationMs(std::uint32_t durationMs) noexcept
{
    m_demoStageDurationMs = std::max(durationMs, 5000U);
}

std::uint32_t QixGame::getDemoDurationMs() const noexcept
{
    return m_demoStageDurationMs;
}

void QixGame::resetDemoPlayfield() noexcept
{
    m_playfield.initBorders();
    m_marker = Marker {Point {m_playfield.getWidth() / 2, m_playfield.getHeight() - 1}, 3};
    m_stats.score = 0;
    m_stats.claimedCells = 0;
    m_stats.claimedPercent = 0;
    m_stats.lives = 3;
    m_stats.level = 1;
    m_stats.multiplier = 1;
    m_stats.splitBonus = false;
    m_stats.thresholdBonus = 0;
    m_stats.qixTrapped = false;
    m_stats.spiralBonus = false;
    m_stats.qixRemainingPercent = 0;
    m_stats.trapBonus = 0;
    m_stats.timeUp = false;
    m_stats.timeRemainingMs = computeLevelTimeMs(1);
    m_demoBot.reset();
    setupEntities();
    updateSnapshot();
}

void QixGame::updateAttractCycle(std::uint32_t deltaMs) noexcept
{
    m_attractStageTimerMs += deltaMs;

    if (m_attractStage == AttractStage::TitleScores) {
        if (m_attractStageTimerMs >= TitleStageDurationMs) {
            m_attractStage = AttractStage::Instructions;
            m_attractStageTimerMs = 0;
        }
        updateSnapshot();
        return;
    }

    if (m_attractStage == AttractStage::Instructions) {
        if (m_attractStageTimerMs >= InstructionsStageDurationMs) {
            m_attractStage = AttractStage::GameplayDemo;
            m_attractStageTimerMs = 0;
            resetDemoPlayfield();
        }
        updateSnapshot();
        return;
    }

    // In GameplayDemo stage:
    if (m_attractStageTimerMs >= m_demoStageDurationMs) {
        m_attractStage = AttractStage::TitleScores;
        m_attractStageTimerMs = 0;
        updateSnapshot();
        return;
    }

    // Advance Demo Bot gameplay:
    const auto botCmd = m_demoBot.update(m_view);
    m_pendingCmd = botCmd;

    const bool wasDrawing = m_marker.isDrawing();
    const auto oldTrailSize = m_marker.getTrail().size();

    const bool moved = m_marker.move(m_playfield, m_pendingCmd);

    if (wasDrawing && moved && m_marker.getTrail().size() > oldTrailSize) {
        const auto currentPos = m_marker.getPosition();
        const auto cellState = m_playfield.getCell(currentPos.x, currentPos.y);

        const bool isClosingCell = (m_mode == GameMode::Classic)
            ? (cellState == CellState::Border)
            : (cellState == CellState::Border || cellState == CellState::ClaimedSlow
                || cellState == CellState::ClaimedFast);

        if (isClosingCell) {
            std::vector<Point> qixPositions {};
            for (const auto& qix : m_qixList) {
                qixPositions.push_back(qix.getHead().start);
            }

            const auto fillRes = m_fill.execute(m_playfield, m_marker.getTrail(), qixPositions, m_marker.getDrawMode(),
                m_stats.targetPercent, m_stats.multiplier);

            m_stats.score += fillRes.pointsAwarded;
            m_stats.claimedCells = fillRes.totalClaimedSoFar;
            m_stats.claimedPercent = fillRes.claimedPercent;
            m_stats.thresholdBonus = fillRes.thresholdBonus;

            m_marker.clearTrail();
            m_fuse.reset();

            if (fillRes.thresholdMet || fillRes.splitOccurred) {
                // Advance to next level if sufficient demonstration time remains
                if (m_demoStageDurationMs > m_attractStageTimerMs + 15000U) {
                    m_stats.level = static_cast<std::uint8_t>(m_stats.level + 1);
                    m_stats.claimedCells = 0;
                    m_stats.claimedPercent = 0;
                    m_playfield.initBorders();
                    m_marker.resetPosition(Point {m_playfield.getWidth() / 2, m_playfield.getHeight() - 1});
                    m_demoBot.reset();
                    setupEntities();
                    updateSnapshot();
                    return;
                }
                m_attractStage = AttractStage::TitleScores;
                m_attractStageTimerMs = 0;
                updateSnapshot();
                return;
            }
        }
    }

    for (auto& qix : m_qixList) {
        qix.update(m_playfield);
    }
    for (auto& sparx : m_sparxList) {
        sparx.update(m_playfield);
    }

    const bool markerActive = moved || m_marker.isPacingWait();
    m_fuse.update(m_marker.isDrawing(), markerActive, m_marker.getTrail());

    const auto collision = CollisionDetector::check(m_marker, m_qixList, m_sparxList, m_fuse);
    if (collision != CollisionEvent::None) {
        clearActiveStix();
        m_fuse.reset();
        // Multi-life continuation: preserve claimed territory and continue demo
        if (m_stats.lives > 1) {
            --m_stats.lives;
            m_marker.resetPosition(Point {m_playfield.getWidth() / 2, m_playfield.getHeight() - 1});
            setupEntities();
            m_demoBot.reset();
        } else {
            m_stats.lives = 0;
            m_attractStage = AttractStage::TitleScores;
            m_attractStageTimerMs = 0;
            updateSnapshot();
            return;
        }
    }

    updateSnapshot();
    m_pendingCmd.direction = Direction::None;
}

bool QixGame::quickSave(const std::string& filepath) const noexcept
{
    const std::string targetPath = filepath.empty() ? SaveSystem::getDefaultFilePath() : filepath;

    GameStateSnapshot snap {};
    snap.version = 1;
    snap.mode = m_mode;
    snap.state = m_state;
    snap.baseDelayMs = m_baseDelayMs;
    snap.currentDelayMs = m_currentDelayMs;
    snap.timeRemainingMs = m_timeRemainingMs;
    snap.nextExtraLifeScore = m_nextExtraLifeScore;
    snap.stats = m_stats;

    // Playfield
    snap.playfield.width = m_playfield.getWidth();
    snap.playfield.height = m_playfield.getHeight();
    snap.playfield.cellsRle = SaveSystem::encodeCellsRle(m_playfield.getCells());

    // Marker
    snap.marker.position = m_marker.getPosition();
    snap.marker.drawMode = m_marker.getDrawMode();
    snap.marker.lives = m_marker.getLives();
    snap.marker.trail = m_marker.getTrail();

    // Qixes
    for (const auto& q : m_qixList) {
        QixSnapshot qs {};
        qs.p1 = q.getP1();
        qs.p2 = q.getP2();
        q.getVelocities(qs.vx1, qs.vy1, qs.vx2, qs.vy2);
        qs.segments = q.getSegments();
        snap.qixes.push_back(std::move(qs));
    }

    // Sparx
    for (const auto& s : m_sparxList) {
        SparxSnapshot ss {};
        ss.position = s.getPosition();
        ss.clockwise = s.isClockwise();
        ss.isSuper = s.isSuper();
        snap.sparxList.push_back(ss);
    }

    // Fuse
    snap.fuse.idleLimit = m_fuse.getIdleLimit();
    snap.fuse.idleCounter = m_fuse.getIdleCounter();
    snap.fuse.isBurning = m_fuse.isBurning();
    snap.fuse.trailIndex = m_fuse.getTrailIndex();
    if (auto p = m_fuse.getPosition()) {
        snap.fuse.position = *p;
    }

    return SaveSystem::saveToFile(targetPath, snap);
}

bool QixGame::quickLoad(const std::string& filepath) noexcept
{
    const std::string targetPath = filepath.empty() ? SaveSystem::getDefaultFilePath() : filepath;

    GameStateSnapshot snap {};
    if (!SaveSystem::loadFromFile(targetPath, snap)) {
        return false;
    }

    if (snap.playfield.width <= 0 || snap.playfield.height <= 0) {
        return false;
    }

    m_mode = snap.mode;
    m_state = snap.state;
    m_baseDelayMs = snap.baseDelayMs;
    m_currentDelayMs = snap.currentDelayMs;
    m_timeRemainingMs = snap.timeRemainingMs;
    m_nextExtraLifeScore = snap.nextExtraLifeScore;
    m_stats = snap.stats;

    // Restore Playfield
    std::vector<CellState> cells {};
    const auto totalExpected
        = static_cast<std::size_t>(snap.playfield.width) * static_cast<std::size_t>(snap.playfield.height);
    if (snap.playfield.width != m_playfield.getWidth() || snap.playfield.height != m_playfield.getHeight()) {
        m_playfield = Playfield {snap.playfield.width, snap.playfield.height};
        m_fill = TerritoryFill {snap.playfield.width, snap.playfield.height};
    }
    if (SaveSystem::decodeCellsRle(snap.playfield.cellsRle, totalExpected, cells)) {
        m_playfield.setCells(cells);
    }

    // Restore Marker
    m_marker.setGameMode(m_mode);
    m_marker.restore(snap.marker.position, snap.marker.drawMode, snap.marker.lives, snap.marker.trail);

    // Restore Qixes
    m_qixList.clear();
    for (const auto& qs : snap.qixes) {
        LineSegment startLine {qs.p1, qs.p2};
        Qix q {startLine, 8};
        q.restore(qs.p1, qs.p2, qs.vx1, qs.vy1, qs.vx2, qs.vy2, qs.segments);
        m_qixList.push_back(std::move(q));
    }

    // Restore Sparx
    m_sparxList.clear();
    for (const auto& ss : snap.sparxList) {
        Sparx s {ss.position, ss.clockwise, m_mode, ss.isSuper};
        s.restore(ss.position, ss.clockwise, ss.isSuper);
        m_sparxList.push_back(std::move(s));
    }

    // Restore Fuse
    m_fuse.restore(
        snap.fuse.idleLimit, snap.fuse.idleCounter, snap.fuse.isBurning, snap.fuse.trailIndex, snap.fuse.position);

    updateSnapshot();
    return true;
}

bool QixGame::isRandomSpawns() const noexcept
{
    return m_randomSpawns;
}

void QixGame::setRandomSpawns(bool enabled) noexcept
{
    m_randomSpawns = enabled;
}

void QixGame::setSpawnSeed(std::uint32_t seed) noexcept
{
    m_spawnRng.seed(seed);
}

bool QixGame::isPaused() const noexcept
{
    return m_isPaused;
}

void QixGame::setPaused(bool paused) noexcept
{
    if (paused) {
        if (m_state == GameState::Playing || m_state == GameState::Ready) {
            m_isPaused = true;
            updateSnapshot();
        }
    } else {
        m_isPaused = false;
        updateSnapshot();
    }
}

void QixGame::togglePause() noexcept
{
    if (m_isPaused) {
        m_isPaused = false;
        updateSnapshot();
    } else if (m_state == GameState::Playing || m_state == GameState::Ready) {
        m_isPaused = true;
        updateSnapshot();
    }
}

} // namespace qix
