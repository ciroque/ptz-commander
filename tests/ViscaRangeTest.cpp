#include <gtest/gtest.h>

#include "ViscaCommands.h"

using cameras::visca::lumensVcA51p;
using cameras::visca::lumensVcA51pLegacy;
using cameras::visca::tenveo30x;
using cameras::visca::ViscaCommands;

TEST(ViscaRange, LinearProfileMapsEndpointsAndHome) {
    const auto profile = lumensVcA51pLegacy();

    EXPECT_EQ(ViscaCommands::panDegreesToVisca(-170.0f, profile), 0x0000);
    EXPECT_EQ(ViscaCommands::panDegreesToVisca(170.0f, profile), 0xFFFF);
    EXPECT_EQ(ViscaCommands::panDegreesToVisca(0.0f, profile), 0x8000);

    EXPECT_EQ(ViscaCommands::panDegreesToVisca(-1000.0f, profile), 0x0000);
    EXPECT_EQ(ViscaCommands::panDegreesToVisca(1000.0f, profile), 0xFFFF);

    EXPECT_NEAR(ViscaCommands::viscaToPanDegrees(0x0000, profile), -170.0f, 0.01f);
    EXPECT_NEAR(ViscaCommands::viscaToPanDegrees(0x8000, profile), 0.0f, 0.01f);
    EXPECT_NEAR(ViscaCommands::viscaToPanDegrees(0xFFFF, profile), 170.0f, 0.01f);
}

TEST(ViscaRange, ThroughHomeSignedProfileUsesTheManualSteps) {
    const auto profile = lumensVcA51p();

    EXPECT_EQ(ViscaCommands::panDegreesToVisca(-170.0f, profile), 0x95C0);
    EXPECT_EQ(ViscaCommands::panDegreesToVisca(0.0f, profile), 0x0000);
    EXPECT_EQ(ViscaCommands::panDegreesToVisca(1.0f, profile), 160);
    EXPECT_EQ(ViscaCommands::panDegreesToVisca(170.0f, profile), 0x6A40);

    EXPECT_EQ(ViscaCommands::tiltDegreesToVisca(-30.0f, profile), 0xED40);
    EXPECT_EQ(ViscaCommands::tiltDegreesToVisca(0.0f, profile), 0x0000);
    EXPECT_EQ(ViscaCommands::tiltDegreesToVisca(90.0f, profile), 0x3840);

    EXPECT_NEAR(ViscaCommands::viscaToPanDegrees(0x95C0, profile), -170.0f, 0.01f);
    EXPECT_NEAR(ViscaCommands::viscaToPanDegrees(0x0000, profile), 0.0f, 0.01f);
    EXPECT_NEAR(ViscaCommands::viscaToPanDegrees(160, profile), 1.0f, 0.01f);
    EXPECT_NEAR(ViscaCommands::viscaToTiltDegrees(0x3840, profile), 90.0f, 0.01f);
}

TEST(ViscaRange, TenveoPanStopsAtTheMeasuredRawPositions) {
    const auto profile = tenveo30x();

    EXPECT_EQ(ViscaCommands::panDegreesToVisca(-175.0f, profile), 0x0000);
    EXPECT_EQ(ViscaCommands::panDegreesToVisca(0.0f, profile), 0x4A6C);
    EXPECT_EQ(ViscaCommands::panDegreesToVisca(175.0f, profile), 0x8E3E);

    EXPECT_EQ(ViscaCommands::tiltDegreesToVisca(-90.0f, profile), 0x0000);
    EXPECT_EQ(ViscaCommands::tiltDegreesToVisca(90.0f, profile), 0x3C8A);
}

TEST(ViscaRange, ZoomPercentClampsAndRoundTrips) {
    const auto profile = lumensVcA51p();

    EXPECT_EQ(ViscaCommands::zoomPercentToVisca(0, profile), 0x0000);
    EXPECT_EQ(ViscaCommands::zoomPercentToVisca(50, profile), 0x2000);
    EXPECT_EQ(ViscaCommands::zoomPercentToVisca(100, profile), 0x4000);
    EXPECT_EQ(ViscaCommands::zoomPercentToVisca(-5, profile), 0x0000);
    EXPECT_EQ(ViscaCommands::zoomPercentToVisca(150, profile), 0x4000);

    EXPECT_EQ(ViscaCommands::viscaToZoomPercent(0x0000, profile), 0);
    EXPECT_EQ(ViscaCommands::viscaToZoomPercent(0x2000, profile), 50);
    EXPECT_EQ(ViscaCommands::viscaToZoomPercent(0x4000, profile), 100);
}

TEST(ViscaRange, ZeroWidthDegreeRangeReturnsTheMinRaw) {
    auto profile = lumensVcA51pLegacy();
    profile.pan.minDeg = 0.0f;
    profile.pan.maxDeg = 0.0f;
    profile.pan.minRaw = 10;
    profile.pan.maxRaw = 20;

    EXPECT_EQ(ViscaCommands::panDegreesToVisca(5.0f, profile), 10);
}

TEST(ViscaRange, ParsesPositionRepliesAndRejectsOtherPackets) {
    const std::vector<uint8_t> panTilt = {0x90, 0x50, 0x01, 0x02, 0x03, 0x04, 0x0A, 0x0B, 0x0C, 0x0D, 0xFF};
    const auto parsed = ViscaCommands::parsePanTiltPosition(panTilt);
    ASSERT_TRUE(parsed);
    EXPECT_EQ(parsed->pan, 0x1234);
    EXPECT_EQ(parsed->tilt, 0xABCD);

    const std::vector<uint8_t> zoom = {0x90, 0x50, 0x00, 0x04, 0x00, 0x00, 0xFF};
    const auto zoomPos = ViscaCommands::parseZoomPosition(zoom);
    ASSERT_TRUE(zoomPos);
    EXPECT_EQ(*zoomPos, 0x0400);

    EXPECT_FALSE(ViscaCommands::parsePanTiltPosition({0x90, 0x50, 0xFF}));
    EXPECT_FALSE(ViscaCommands::parseZoomPosition({0x90, 0x60, 0x02, 0xFF}));
}
