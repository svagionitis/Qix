#include "SpeedConfig.h"
#include <gtest/gtest.h>

TEST(SpeedConfigTest, DefaultDelayAndBoundsClamping)
{
    EXPECT_EQ(qix::SpeedConfig::DefaultDelayMs, 75U);
    EXPECT_EQ(qix::SpeedConfig::MinDelayMs, 20U);
    EXPECT_EQ(qix::SpeedConfig::MaxDelayMs, 250U);
    EXPECT_EQ(qix::SpeedConfig::StepDelayMs, 10U);

    EXPECT_EQ(qix::SpeedConfig::clampDelay(10U), 20U);
    EXPECT_EQ(qix::SpeedConfig::clampDelay(20U), 20U);
    EXPECT_EQ(qix::SpeedConfig::clampDelay(75U), 75U);
    EXPECT_EQ(qix::SpeedConfig::clampDelay(250U), 250U);
    EXPECT_EQ(qix::SpeedConfig::clampDelay(300U), 250U);
}

TEST(SpeedConfigTest, SpeedUpAndSpeedDown)
{
    // Intermediate pacing
    EXPECT_EQ(qix::SpeedConfig::speedUp(75U), 65U);
    EXPECT_EQ(qix::SpeedConfig::speedDown(75U), 85U);

    // Speed up approaching and at lower limit
    EXPECT_EQ(qix::SpeedConfig::speedUp(25U), 20U);
    EXPECT_EQ(qix::SpeedConfig::speedUp(20U), 20U);

    // Speed down approaching and at upper limit
    EXPECT_EQ(qix::SpeedConfig::speedDown(245U), 250U);
    EXPECT_EQ(qix::SpeedConfig::speedDown(250U), 250U);
}

TEST(SpeedConfigTest, ParseDelayFlags)
{
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"--delay", "100"}), 100U);
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"-d", "50"}), 50U);

    // Clamping on extreme values
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"--delay", "5"}), 20U);
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"-d", "500"}), 250U);
}

TEST(SpeedConfigTest, ParseFpsFlags)
{
    // 20 FPS -> 1000 / 20 = 50ms
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"--fps", "20"}), 50U);

    // 10 FPS -> 1000 / 10 = 100ms
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"-f", "10"}), 100U);

    // 60 FPS -> 1000 / 60 = 16ms -> clamped to 20ms
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"--fps", "60"}), 20U);

    // 0 FPS -> invalid, retains default
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"--fps", "0"}), qix::SpeedConfig::DefaultDelayMs);
}

TEST(SpeedConfigTest, ParseArgcArgvArray)
{
    char arg0[] = "qix_gui";
    char arg1[] = "-d";
    char arg2[] = "90";
    char* argv[] = {arg0, arg1, arg2};

    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs(3, argv), 90U);
}

TEST(SpeedConfigTest, ParseMalformedOrEmptyArgs)
{
    // Empty vector
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({}), qix::SpeedConfig::DefaultDelayMs);

    // Missing value
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"--delay"}), qix::SpeedConfig::DefaultDelayMs);
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"--fps"}), qix::SpeedConfig::DefaultDelayMs);

    // Non-numeric value
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"--delay", "invalid"}), qix::SpeedConfig::DefaultDelayMs);
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"-f", "not_a_number"}), qix::SpeedConfig::DefaultDelayMs);

    // Unrelated args
    EXPECT_EQ(qix::SpeedConfig::parseSpeedArgs({"--verbose", "-v"}), qix::SpeedConfig::DefaultDelayMs);
}
