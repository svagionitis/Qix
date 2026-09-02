#include "QixGame.h"

namespace qix {

QixGame::QixGame(std::int32_t width, std::int32_t height, std::uint16_t targetPercent) noexcept
    : m_playfield {width, height}
    , m_marker {Point {width / 2, height - 1}, 3}
    , m_fuse {25}
    , m_fill {width, height}
{
    m_stats.targetPercent = targetPercent;
    m_stats.totalEmptyCells = m_playfield.getInteriorCount();
    reset();
}

void QixGame::step(std::uint32_t deltaMs) noexcept
{
    static_cast<void>(deltaMs);

    if (m_state == GameState::GameOver || m_state == GameState::LevelComplete) {
        updateSnapshot();
        return;
    }

    if (m_state == GameState::Ready) {
        if (m_pendingCmd.direction != Direction::None) {
            m_state = GameState::Playing;
        } else {
            updateSnapshot();
            return;
        }
    }

    const bool wasDrawing = m_marker.isDrawing();
    const auto oldTrailSize = m_marker.getTrail().size();

    // 1. Move Player Marker
    const bool moved = m_marker.move(m_playfield, m_pendingCmd);

    // 2. Loop Closure Check: Marker was drawing and returned to a border/claimed cell
    if (wasDrawing && moved && m_marker.getTrail().size() > oldTrailSize) {
        const auto currentPos = m_marker.getPosition();
        const auto cellState = m_playfield.getCell(currentPos.x, currentPos.y);

        // If the marker reached border or claimed territory after stepping into empty space
        if (cellState == CellState::Border || cellState == CellState::ClaimedSlow
            || cellState == CellState::ClaimedFast) {
            std::vector<Point> qixPositions {};
            for (const auto& qix : m_qixList) {
                qixPositions.push_back(qix.getHead().start);
            }

            const auto fillRes = m_fill.execute(
                m_playfield, m_marker.getTrail(), qixPositions, m_marker.getDrawMode(), m_stats.targetPercent);

            m_stats.score += fillRes.pointsAwarded;
            m_stats.claimedCells = fillRes.totalClaimedSoFar;
            m_stats.claimedPercent = fillRes.claimedPercent;

            m_marker.clearTrail();
            m_fuse.reset();

            if (fillRes.thresholdMet) {
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
    m_fuse.update(m_marker.isDrawing(), moved, m_marker.getTrail());

    // 6. Audit Collisions
    const auto collision = CollisionDetector::check(m_marker, m_qixList, m_sparxList, m_fuse);
    if (collision != CollisionEvent::None) {
        handleDeath();
    }

    // 7. Refresh View Snapshot
    updateSnapshot();

    // Consume single-tick input command
    m_pendingCmd.direction = Direction::None;
}

void QixGame::handleInput(PlayerCommand cmd) noexcept
{
    m_pendingCmd = cmd;
}

const GameView& QixGame::getView() const noexcept
{
    return m_view;
}

void QixGame::reset() noexcept
{
    m_playfield.initBorders();
    m_marker = Marker {Point {m_playfield.getWidth() / 2, m_playfield.getHeight() - 1}, 3};
    m_stats.score = 0;
    m_stats.claimedCells = 0;
    m_stats.claimedPercent = 0;
    m_stats.lives = 3;
    m_stats.level = 1;
    m_state = GameState::Ready;

    setupEntities();
    updateSnapshot();
}

void QixGame::nextLevel() noexcept
{
    m_playfield.initBorders();
    m_marker.resetPosition(Point {m_playfield.getWidth() / 2, m_playfield.getHeight() - 1});
    m_stats.claimedCells = 0;
    m_stats.claimedPercent = 0;
    ++m_stats.level;
    m_state = GameState::Ready;

    setupEntities();
    updateSnapshot();
}

void QixGame::setupEntities() noexcept
{
    m_qixList.clear();

    // Spawn Qix near center of empty field
    const auto cx = m_playfield.getWidth() / 2;
    const auto cy = m_playfield.getHeight() / 2;
    LineSegment qixLine {Point {cx - 5, cy}, Point {cx + 5, cy}};
    m_qixList.emplace_back(qixLine, 10);

    // If level 2 or higher, add a second Qix
    if (m_stats.level >= 2) {
        LineSegment qixLine2 {Point {cx, cy - 5}, Point {cx, cy + 5}};
        m_qixList.emplace_back(qixLine2, 10);
    }

    m_sparxList.clear();
    // Sparx 1: Clockwise from top-left
    m_sparxList.emplace_back(Point {1, 0}, true);
    // Sparx 2: Counter-clockwise from top-right
    m_sparxList.emplace_back(Point {m_playfield.getWidth() - 2, 0}, false);

    m_fuse.reset();
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
    for (const auto& sparx : m_sparxList) {
        m_view.sparxPositions.push_back(sparx.getPosition());
    }

    m_view.fusePos = m_fuse.getPosition();
    m_stats.lives = m_marker.getLives();
    m_view.stats = m_stats;
    m_view.state = m_state;
}

void QixGame::handleDeath() noexcept
{
    m_marker.decrementLives();
    clearActiveStix();
    m_fuse.reset();

    if (!m_marker.isAlive()) {
        m_state = GameState::GameOver;
    } else {
        // Respawn marker at bottom safe border
        m_marker.resetPosition(Point {m_playfield.getWidth() / 2, m_playfield.getHeight() - 1});
        m_state = GameState::Ready;
    }
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

} // namespace qix
