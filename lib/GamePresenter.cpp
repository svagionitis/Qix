#include "GamePresenter.h"
#include <algorithm>
#include <cstdio>

namespace qix {

const std::array<InstructionRule, 5>& GamePresenter::getInstructionRules() noexcept
{
    static const std::array<InstructionRule, 5> s_rules {
        {{"OBJECTIVE:", "CLAIM 75% OF THE PLAYFIELD TO WIN", PaletteColor {59, 130, 246, 255}},
            {"SLOW DRAW:", "HOLD [SPACE] WHILE MOVING (2X POINTS)", PaletteColor {34, 197, 94, 255}},
            {"FAST DRAW:", "HOLD [SHIFT/F] WHILE MOVING (1X POINTS)", PaletteColor {245, 158, 11, 255}},
            {"HAZARDS:", "AVOID BOUNCING QIX AND SPARX ON BORDERS", PaletteColor {239, 68, 68, 255}},
            {"THE FUSE:", "BURNS YOUR TRAIL IF YOU STOP MOVING!", PaletteColor {217, 70, 239, 255}}}};
    return s_rules;
}

VictoryPresentation GamePresenter::formatVictory(const GameStats& stats) noexcept
{
    VictoryPresentation pres {};
    if (stats.qixTrapped) {
        pres.title = stats.spiralBonus ? "*** SPIRAL QIX TRAP! ***" : "*** QIX TRAPPED! ***";
        pres.titleColor = PaletteColor {250, 204, 21, 255};
        pres.detail = "QIX CONFINED TO " + std::to_string(stats.qixRemainingPercent) + "% OF FIELD!";
        pres.hasDetail = true;
        pres.bonus = "+" + std::to_string(stats.trapBonus) + " TRAP BONUS!"
            + (stats.thresholdBonus > 0 ? (" (+" + std::to_string(stats.thresholdBonus) + " OVERSHOOT)") : "");
        pres.hasBonus = true;
        pres.prompt = "Press [Space] for Next Level";
        pres.isTrap = true;
    } else {
        pres.title = stats.splitBonus ? "QIX SPLIT BONUS!" : "LEVEL COMPLETE!";
        pres.titleColor = stats.splitBonus ? PaletteColor {250, 204, 21, 255} : PaletteColor {74, 222, 128, 255};
        if (!stats.splitBonus && stats.thresholdBonus > 0) {
            const auto overshoot
                = (stats.claimedPercent > stats.targetPercent) ? (stats.claimedPercent - stats.targetPercent) : 0U;
            pres.bonus
                = "+" + std::to_string(stats.thresholdBonus) + " THRESHOLD BONUS (+" + std::to_string(overshoot) + "%)";
            pres.hasBonus = true;
        }
        pres.prompt = stats.splitBonus ? ("Multiplier: " + std::to_string(stats.multiplier) + "X! Press [Space]")
                                       : "Press [Space] for Next Level";
        pres.hasDetail = false;
        pres.isTrap = false;
    }
    return pres;
}

HudPresentation GamePresenter::formatHud(const GameStats& stats, std::uint32_t delayMs) noexcept
{
    HudPresentation pres {};
    pres.scoreStr = std::to_string(stats.score);
    pres.hiScoreStr = std::to_string(stats.highScore);
    pres.claimStr = std::to_string(stats.claimedPercent) + "% / " + std::to_string(stats.targetPercent) + "%";
    pres.targetReached = (stats.claimedPercent >= stats.targetPercent);
    pres.progressRatio = std::min(1.0f, static_cast<float>(stats.claimedPercent) / 100.0f);
    pres.fillPercent = std::min(100, static_cast<int>(stats.claimedPercent));

    pres.secondsRemaining = (stats.timeRemainingMs + 999U) / 1000U;
    pres.timeStr = std::to_string(pres.secondsRemaining) + "s";
    if (stats.timeUp || pres.secondsRemaining <= 10U) {
        pres.timeUrgency = HudUrgency::Critical;
    } else if (pres.secondsRemaining <= 20U) {
        pres.timeUrgency = HudUrgency::Warning;
    } else {
        pres.timeUrgency = HudUrgency::Normal;
    }

    if (stats.multiplier > 1) {
        pres.multiplierStr = std::to_string(stats.multiplier) + "X";
    }
    pres.speedStr = std::to_string(delayMs) + "ms";
    pres.levelStr = std::to_string(stats.level);

    return pres;
}

std::vector<HofRowPresentation> GamePresenter::formatHallOfFame(
    const HighScoreTable* table, std::size_t maxRows) noexcept
{
    std::vector<HofRowPresentation> rows;
    if (!table) {
        return rows;
    }

    const auto& entries = table->getEntries();
    const std::size_t count = std::min(entries.size(), maxRows);
    rows.reserve(count);

    for (std::size_t i {0}; i < count; ++i) {
        const auto& e = entries[i];
        HofRowPresentation row {};
        row.rank = i + 1;

        if (i == 0) {
            row.medal = MedalTier::Gold;
            row.medalColor = PaletteColor {250, 204, 21, 255};
        } else if (i == 1) {
            row.medal = MedalTier::Silver;
            row.medalColor = PaletteColor {226, 232, 240, 255};
        } else if (i == 2) {
            row.medal = MedalTier::Bronze;
            row.medalColor = PaletteColor {245, 158, 11, 255};
        } else {
            row.medal = MedalTier::Standard;
            row.medalColor = PaletteColor {148, 163, 184, 255};
        }

        char rowBuf[64];
        const char* modeStr = (e.mode == GameMode::Classic) ? "CLASSIC" : "MODERN";
        std::snprintf(rowBuf, sizeof(rowBuf), "%2zu.      %-4s    %8u      %02u     %-7s", i + 1, e.initials.c_str(),
            e.score, static_cast<unsigned>(e.level), modeStr);
        row.formattedRow = rowBuf;

        rows.push_back(std::move(row));
    }
    return rows;
}

std::string GamePresenter::formatHofPrompt(bool isAttract, bool blink) noexcept
{
    if (isAttract) {
        return blink ? "INSERT COIN - PRESS [SPACE] TO PLAY" : "";
    }
    return "PRESS [R] OR [SPACE] TO PLAY AGAIN";
}

std::string GamePresenter::formatRecordBanner(std::size_t rank, std::uint32_t score) noexcept
{
    return "NEW RECORD! RANK #" + std::to_string(rank) + " - SCORE: " + std::to_string(score);
}

} // namespace qix
