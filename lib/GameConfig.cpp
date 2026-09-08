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

bool GameConfig::parseCrtFlag(const std::vector<std::string>& args, bool defaultCrt) noexcept
{
    bool crt {defaultCrt};

    for (const auto& rawArg : args) {
        const auto arg = toLower(rawArg);
        if (arg == "--crt" || arg == "--scanlines" || arg == "-c") {
            crt = true;
        } else if (arg == "--no-crt" || arg == "--no-scanlines") {
            crt = false;
        }
    }

    return crt;
}

bool GameConfig::parseCrtFlag(int argc, char* const argv[], bool defaultCrt) noexcept
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
    return parseCrtFlag(args, defaultCrt);
}

bool GameConfig::parseAudioFlag(const std::vector<std::string>& args, bool defaultAudio) noexcept
{
    bool audio {defaultAudio};

    for (const auto& rawArg : args) {
        const auto arg = toLower(rawArg);
        if (arg == "--audio" || arg == "--sound" || arg == "-s") {
            audio = true;
        } else if (arg == "--no-audio" || arg == "--no-sound") {
            audio = false;
        }
    }

    return audio;
}

bool GameConfig::parseAudioFlag(int argc, char* const argv[], bool defaultAudio) noexcept
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
    return parseAudioFlag(args, defaultAudio);
}

PaletteId GameConfig::parsePaletteFlag(const std::vector<std::string>& args, PaletteId defaultPalette) noexcept
{
    PaletteId palette {defaultPalette};

    for (std::size_t i {0}; i < args.size(); ++i) {
        const auto arg = toLower(args[i]);

        if (arg.rfind("--palette=", 0) == 0) {
            palette = ColorPalette::fromName(arg.substr(10), palette);
        } else if (arg.rfind("-p=", 0) == 0) {
            palette = ColorPalette::fromName(arg.substr(3), palette);
        } else if ((arg == "--palette" || arg == "-p") && i + 1 < args.size()) {
            palette = ColorPalette::fromName(args[++i], palette);
        } else if (arg == "--classic") {
            // Note: --classic could be game mode or palette, but if specified standalone it fits both
            palette = PaletteId::Classic;
        } else if (arg == "--synthwave" || arg == "--cyberpunk" || arg == "--neon") {
            palette = PaletteId::Synthwave;
        } else if (arg == "--amber" || arg == "--p3") {
            palette = PaletteId::Amber;
        } else if (arg == "--green" || arg == "--p1") {
            palette = PaletteId::Green;
        }
    }

    return palette;
}

PaletteId GameConfig::parsePaletteFlag(int argc, char* const argv[], PaletteId defaultPalette) noexcept
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
    return parsePaletteFlag(args, defaultPalette);
}

bool GameConfig::parseAttractFlag(const std::vector<std::string>& args, bool defaultAttract) noexcept
{
    bool attract {defaultAttract};

    for (const auto& rawArg : args) {
        const auto arg = toLower(rawArg);
        if (arg == "--attract" || arg == "--demo" || arg == "-d") {
            attract = true;
        } else if (arg == "--no-attract" || arg == "--no-demo") {
            attract = false;
        }
    }

    return attract;
}

bool GameConfig::parseAttractFlag(int argc, char* const argv[], bool defaultAttract) noexcept
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
    return parseAttractFlag(args, defaultAttract);
}

} // namespace qix
