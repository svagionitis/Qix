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

std::uint32_t GameConfig::parseDemoDurationFlag(
    const std::vector<std::string>& args, std::uint32_t defaultSeconds) noexcept
{
    std::uint32_t seconds {defaultSeconds};

    for (std::size_t i {0}; i < args.size(); ++i) {
        const auto arg = toLower(args[i]);
        if (arg == "--demo-duration" || arg == "--attract-duration") {
            if (i + 1 < args.size()) {
                try {
                    const int val = std::stoi(args[i + 1]);
                    if (val > 0) {
                        seconds = static_cast<std::uint32_t>(val);
                    }
                } catch (...) {
                }
            }
        } else if (arg.rfind("--demo-duration=", 0) == 0) {
            try {
                const int val = std::stoi(arg.substr(16));
                if (val > 0) {
                    seconds = static_cast<std::uint32_t>(val);
                }
            } catch (...) {
            }
        } else if (arg.rfind("--attract-duration=", 0) == 0) {
            try {
                const int val = std::stoi(arg.substr(19));
                if (val > 0) {
                    seconds = static_cast<std::uint32_t>(val);
                }
            } catch (...) {
            }
        }
    }

    return seconds * 1000U;
}

std::uint32_t GameConfig::parseDemoDurationFlag(int argc, char* const argv[], std::uint32_t defaultSeconds) noexcept
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
    return parseDemoDurationFlag(args, defaultSeconds);
}

bool GameConfig::parseArtFlag(const std::vector<std::string>& args, bool defaultArt) noexcept
{
    bool art {defaultArt};

    for (const auto& rawArg : args) {
        const auto arg = toLower(rawArg);
        if (arg == "--art" || arg == "--bg-art" || arg == "--reveal") {
            art = true;
        } else if (arg == "--no-art" || arg == "--no-bg-art" || arg == "--no-reveal") {
            art = false;
        }
    }

    return art;
}

bool GameConfig::parseArtFlag(int argc, char* const argv[], bool defaultArt) noexcept
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
    return parseArtFlag(args, defaultArt);
}

int GameConfig::parseArtSceneFlag(const std::vector<std::string>& args, int defaultScene) noexcept
{
    int scene {defaultScene};

    for (std::size_t i {0}; i < args.size(); ++i) {
        const auto arg = toLower(args[i]);
        if (arg.rfind("--art-scene=", 0) == 0) {
            try {
                scene = std::stoi(arg.substr(12));
            } catch (...) {
            }
        } else if (arg.rfind("--scene=", 0) == 0) {
            try {
                scene = std::stoi(arg.substr(8));
            } catch (...) {
            }
        } else if ((arg == "--art-scene" || arg == "--scene") && i + 1 < args.size()) {
            try {
                scene = std::stoi(args[++i]);
            } catch (...) {
            }
        }
    }

    return scene;
}

int GameConfig::parseArtSceneFlag(int argc, char* const argv[], int defaultScene) noexcept
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
    return parseArtSceneFlag(args, defaultScene);
}

std::string GameConfig::parseRecordFlag(const std::vector<std::string>& args) noexcept
{
    for (std::size_t i {0}; i < args.size(); ++i) {
        const auto& rawArg = args[i];
        if (rawArg.rfind("--record=", 0) == 0) {
            return rawArg.substr(9);
        }
        if ((rawArg == "--record" || rawArg == "-r") && i + 1 < args.size()) {
            return args[i + 1];
        }
    }
    return "";
}

std::string GameConfig::parseRecordFlag(int argc, char* const argv[]) noexcept
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
    return parseRecordFlag(args);
}

std::string GameConfig::parseReplayFlag(const std::vector<std::string>& args) noexcept
{
    for (std::size_t i {0}; i < args.size(); ++i) {
        const auto& rawArg = args[i];
        if (rawArg.rfind("--replay=", 0) == 0) {
            return rawArg.substr(9);
        }
        if (rawArg.rfind("--playback=", 0) == 0) {
            return rawArg.substr(11);
        }
        if ((rawArg == "--replay" || rawArg == "--playback") && i + 1 < args.size()) {
            return args[i + 1];
        }
    }
    return "";
}

std::string GameConfig::parseReplayFlag(int argc, char* const argv[]) noexcept
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
    return parseReplayFlag(args);
}

} // namespace qix
