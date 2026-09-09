#include "GameConfig.h"
#include <gtest/gtest.h>
#include <vector>

TEST(GameConfigTest, DefaultGameModeValue)
{
    EXPECT_EQ(qix::GameConfig::parseGameMode({}), qix::DefaultGameMode);
    EXPECT_EQ(qix::GameConfig::parseGameMode({}, qix::GameMode::Modern), qix::GameMode::Modern);
    EXPECT_EQ(qix::GameConfig::parseGameMode({}, qix::GameMode::Classic), qix::GameMode::Classic);
}

TEST(GameConfigTest, ParseClassicFlags)
{
    EXPECT_EQ(qix::GameConfig::parseGameMode({"--classic"}), qix::GameMode::Classic);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"--mode", "classic"}), qix::GameMode::Classic);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"--mode", "Classic"}), qix::GameMode::Classic);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"--mode", "CLASSIC"}), qix::GameMode::Classic);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"-m", "classic"}), qix::GameMode::Classic);
}

TEST(GameConfigTest, ParseModernFlags)
{
    EXPECT_EQ(qix::GameConfig::parseGameMode({"--modern"}), qix::GameMode::Modern);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"--mode", "modern"}), qix::GameMode::Modern);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"--mode", "Modern"}), qix::GameMode::Modern);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"--mode", "MODERN"}), qix::GameMode::Modern);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"-m", "modern"}), qix::GameMode::Modern);
}

TEST(GameConfigTest, ParseArgcArgvArray)
{
    char arg0[] = "qix_app";
    char arg1[] = "--mode";
    char arg2[] = "modern";
    char* argv[] = {arg0, arg1, arg2, nullptr};

    EXPECT_EQ(qix::GameConfig::parseGameMode(3, argv), qix::GameMode::Modern);
}

TEST(GameConfigTest, MalformedOrUnknownArgs)
{
    EXPECT_EQ(
        qix::GameConfig::parseGameMode({"--mode", "unsupported"}, qix::GameMode::Classic), qix::GameMode::Classic);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"--mode"}, qix::GameMode::Classic), qix::GameMode::Classic);
    EXPECT_EQ(qix::GameConfig::parseGameMode({"-m"}, qix::GameMode::Modern), qix::GameMode::Modern);
    EXPECT_EQ(qix::GameConfig::parseGameMode(0, nullptr), qix::DefaultGameMode);
}

TEST(GameConfigTest, ToStringRepresentation)
{
    EXPECT_STREQ(qix::GameConfig::toString(qix::GameMode::Classic), "Classic");
    EXPECT_STREQ(qix::GameConfig::toString(qix::GameMode::Modern), "Modern");
}

TEST(GameConfigTest, ParseCrtFlags)
{
    EXPECT_FALSE(qix::GameConfig::parseCrtFlag({}));
    EXPECT_TRUE(qix::GameConfig::parseCrtFlag({}, true));

    EXPECT_TRUE(qix::GameConfig::parseCrtFlag({"--crt"}));
    EXPECT_TRUE(qix::GameConfig::parseCrtFlag({"--scanlines"}));
    EXPECT_TRUE(qix::GameConfig::parseCrtFlag({"-c"}));
    EXPECT_TRUE(qix::GameConfig::parseCrtFlag({"--CRT"}));

    EXPECT_FALSE(qix::GameConfig::parseCrtFlag({"--no-crt"}));
    EXPECT_FALSE(qix::GameConfig::parseCrtFlag({"--no-scanlines"}));
    EXPECT_FALSE(qix::GameConfig::parseCrtFlag({"--crt", "--no-crt"}));
    EXPECT_TRUE(qix::GameConfig::parseCrtFlag({"--no-crt", "--crt"}));

    char arg0[] = "qix_app";
    char arg1[] = "--crt";
    char* argv[] = {arg0, arg1, nullptr};
    EXPECT_TRUE(qix::GameConfig::parseCrtFlag(2, argv));
    EXPECT_FALSE(qix::GameConfig::parseCrtFlag(0, nullptr));
}

TEST(GameConfigTest, ParseAudioFlags)
{
    EXPECT_FALSE(qix::GameConfig::parseAudioFlag({}));
    EXPECT_TRUE(qix::GameConfig::parseAudioFlag({}, true));

    EXPECT_TRUE(qix::GameConfig::parseAudioFlag({"--audio"}));
    EXPECT_TRUE(qix::GameConfig::parseAudioFlag({"--sound"}));
    EXPECT_TRUE(qix::GameConfig::parseAudioFlag({"-s"}));
    EXPECT_TRUE(qix::GameConfig::parseAudioFlag({"--AUDIO"}));

    EXPECT_FALSE(qix::GameConfig::parseAudioFlag({"--no-audio"}));
    EXPECT_FALSE(qix::GameConfig::parseAudioFlag({"--no-sound"}));
    EXPECT_FALSE(qix::GameConfig::parseAudioFlag({"--audio", "--no-audio"}));
    EXPECT_TRUE(qix::GameConfig::parseAudioFlag({"--no-audio", "--audio"}));

    char arg0[] = "qix_app";
    char arg1[] = "--sound";
    char* argv[] = {arg0, arg1, nullptr};
    EXPECT_TRUE(qix::GameConfig::parseAudioFlag(2, argv));
    EXPECT_FALSE(qix::GameConfig::parseAudioFlag(0, nullptr));
}

TEST(GameConfigTest, ParsePaletteFlags)
{
    EXPECT_EQ(qix::GameConfig::parsePaletteFlag({}), qix::PaletteId::Classic);
    EXPECT_EQ(qix::GameConfig::parsePaletteFlag({}, qix::PaletteId::Amber), qix::PaletteId::Amber);

    EXPECT_EQ(qix::GameConfig::parsePaletteFlag({"--palette=synthwave"}), qix::PaletteId::Synthwave);
    EXPECT_EQ(qix::GameConfig::parsePaletteFlag({"--palette", "green"}), qix::PaletteId::Green);
    EXPECT_EQ(qix::GameConfig::parsePaletteFlag({"-p", "amber"}), qix::PaletteId::Amber);
    EXPECT_EQ(qix::GameConfig::parsePaletteFlag({"-p=synthwave"}), qix::PaletteId::Synthwave);

    EXPECT_EQ(qix::GameConfig::parsePaletteFlag({"--neon"}), qix::PaletteId::Synthwave);
    EXPECT_EQ(qix::GameConfig::parsePaletteFlag({"--amber"}), qix::PaletteId::Amber);
    EXPECT_EQ(qix::GameConfig::parsePaletteFlag({"--green"}), qix::PaletteId::Green);

    char arg0[] = "qix_app";
    char arg1[] = "-p";
    char arg2[] = "amber";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    EXPECT_EQ(qix::GameConfig::parsePaletteFlag(3, argv), qix::PaletteId::Amber);
    EXPECT_EQ(qix::GameConfig::parsePaletteFlag(0, nullptr), qix::PaletteId::Classic);
}

TEST(GameConfigTest, ParseArtFlags)
{
    EXPECT_TRUE(qix::GameConfig::parseArtFlag(std::vector<std::string> {"--art"}));
    EXPECT_TRUE(qix::GameConfig::parseArtFlag(std::vector<std::string> {"--bg-art"}));
    EXPECT_TRUE(qix::GameConfig::parseArtFlag(std::vector<std::string> {"--reveal"}));
    EXPECT_FALSE(qix::GameConfig::parseArtFlag(std::vector<std::string> {"--no-art"}));
    EXPECT_FALSE(qix::GameConfig::parseArtFlag(std::vector<std::string> {"--no-bg-art"}));
    EXPECT_FALSE(qix::GameConfig::parseArtFlag(std::vector<std::string> {"--no-reveal"}));

    // Default fallback
    EXPECT_TRUE(qix::GameConfig::parseArtFlag(std::vector<std::string> {}, true));
    EXPECT_FALSE(qix::GameConfig::parseArtFlag(std::vector<std::string> {}, false));

    // Argc/argv parsing
    char arg0[] = "qix";
    char arg1[] = "--no-art";
    char* argv[] = {arg0, arg1, nullptr};
    EXPECT_FALSE(qix::GameConfig::parseArtFlag(2, argv, true));
}

TEST(GameConfigTest, ParseArtSceneFlags)
{
    EXPECT_EQ(qix::GameConfig::parseArtSceneFlag(std::vector<std::string> {"--art-scene=2"}), 2);
    EXPECT_EQ(qix::GameConfig::parseArtSceneFlag(std::vector<std::string> {"--scene=1"}), 1);
    EXPECT_EQ(qix::GameConfig::parseArtSceneFlag(std::vector<std::string> {"--art-scene", "3"}), 3);
    EXPECT_EQ(qix::GameConfig::parseArtSceneFlag(std::vector<std::string> {"--scene", "0"}), 0);
    EXPECT_EQ(qix::GameConfig::parseArtSceneFlag(std::vector<std::string> {}, -1), -1);
}

TEST(GameConfigTest, ParseRandomSpawnsFlags)
{
    EXPECT_TRUE(qix::GameConfig::parseRandomSpawnsFlag(std::vector<std::string> {"--random-spawns"}));
    EXPECT_TRUE(qix::GameConfig::parseRandomSpawnsFlag(std::vector<std::string> {"--random-spawn"}));
    EXPECT_TRUE(qix::GameConfig::parseRandomSpawnsFlag(std::vector<std::string> {"--randomize-spawns"}));
    EXPECT_FALSE(qix::GameConfig::parseRandomSpawnsFlag(std::vector<std::string> {"--no-random-spawns"}));
    EXPECT_FALSE(qix::GameConfig::parseRandomSpawnsFlag(std::vector<std::string> {"--no-random-spawn"}));
    EXPECT_FALSE(qix::GameConfig::parseRandomSpawnsFlag(std::vector<std::string> {"--fixed-spawns"}));

    // Default fallback
    EXPECT_TRUE(qix::GameConfig::parseRandomSpawnsFlag(std::vector<std::string> {}, true));
    EXPECT_FALSE(qix::GameConfig::parseRandomSpawnsFlag(std::vector<std::string> {}, false));

    // Argc/argv parsing
    char arg0[] = "qix";
    char arg1[] = "--random-spawns";
    char* argv[] = {arg0, arg1, nullptr};
    EXPECT_TRUE(qix::GameConfig::parseRandomSpawnsFlag(2, argv, false));
}
