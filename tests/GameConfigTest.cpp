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
