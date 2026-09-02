#include "SpeedConfig.h"
#include <algorithm>

namespace qix {

std::uint32_t SpeedConfig::parseSpeedArgs(const std::vector<std::string>& args, std::uint32_t defaultDelay) noexcept
{
    std::uint32_t delayMs {defaultDelay};

    for (std::size_t i {0}; i < args.size(); ++i) {
        const auto& arg = args[i];
        if ((arg == "--delay" || arg == "-d") && (i + 1 < args.size())) {
            try {
                delayMs = static_cast<std::uint32_t>(std::stoul(args[++i]));
            } catch (...) {
                // Ignore invalid input and retain previous setting
            }
        } else if ((arg == "--fps" || arg == "-f") && (i + 1 < args.size())) {
            try {
                const auto fps = std::stoul(args[++i]);
                if (fps > 0) {
                    delayMs = static_cast<std::uint32_t>(1000 / fps);
                }
            } catch (...) {
                // Ignore invalid input and retain previous setting
            }
        }
    }

    return clampDelay(delayMs);
}

std::uint32_t SpeedConfig::parseSpeedArgs(int argc, char* const argv[], std::uint32_t defaultDelay) noexcept
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
    return parseSpeedArgs(args, defaultDelay);
}

} // namespace qix
