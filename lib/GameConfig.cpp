#include "GameConfig.h"
#include <algorithm>
#include <cctype>

namespace qix {

namespace {

    [[nodiscard]] std::string toLower(std::string str) noexcept
    {
        std::transform(
            str.begin(), str.end(), str.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return str;
    }

} // namespace

GameMode GameConfig::parseGameMode(const std::vector<std::string>& args, GameMode defaultMode) noexcept
{
    GameMode mode {defaultMode};

    for (std::size_t i {0}; i < args.size(); ++i) {
        const auto& arg = args[i];
        if (arg == "--classic") {
            mode = GameMode::Classic;
        } else if (arg == "--modern") {
            mode = GameMode::Modern;
        } else if ((arg == "--mode" || arg == "-m") && (i + 1 < args.size())) {
            const auto val = toLower(args[++i]);
            if (val == "classic") {
                mode = GameMode::Classic;
            } else if (val == "modern") {
                mode = GameMode::Modern;
            }
        }
    }

    return mode;
}

GameMode GameConfig::parseGameMode(int argc, char* const argv[], GameMode defaultMode) noexcept
{
    std::vector<std::string> args {};
    if (argc > 1 && argv != nullptr) {
        args.reserve(static_cast<std::size_t>(argc - 1));
        for (int i {1}; i < argc; ++i) {
            if (argv[i] != nullptr) {
                args.emplace_back(argv[i]);
            }
        }
    }
    return parseGameMode(args, defaultMode);
}

const char* GameConfig::toString(GameMode mode) noexcept
{
    switch (mode) {
    case GameMode::Classic:
        return "Classic";
    case GameMode::Modern:
        return "Modern";
    default:
        return "Unknown";
    }
}

} // namespace qix
