#include <gtest/gtest.h>

#include "ViscaIpFraming.h"

using cameras::visca::viscaIpFrame;
using cameras::visca::viscaIpUnframe;

TEST(ViscaIpFraming, CommandGetsType0100AndABigEndianSequence) {
    const std::vector<uint8_t> visca = {0x81, 0x01, 0x06, 0x04, 0xFF};
    const auto framed = viscaIpFrame(visca, 1);

    const std::vector<uint8_t> expected
        = {0x01, 0x00, 0x00, 0x05, 0x00, 0x00, 0x00, 0x01, 0x81, 0x01, 0x06, 0x04, 0xFF};
    EXPECT_EQ(framed, expected);
    EXPECT_EQ(viscaIpUnframe(framed), visca);
}

TEST(ViscaIpFraming, InquiryUsesType0110) {
    const std::vector<uint8_t> visca = {0x81, 0x09, 0x06, 0x12, 0xFF};
    const auto framed = viscaIpFrame(visca, 0x01020304);

    EXPECT_EQ(framed[0], 0x01);
    EXPECT_EQ(framed[1], 0x10);
    EXPECT_EQ(framed[2], 0x00);
    EXPECT_EQ(framed[3], 0x05);
    EXPECT_EQ(framed[4], 0x01);
    EXPECT_EQ(framed[5], 0x02);
    EXPECT_EQ(framed[6], 0x03);
    EXPECT_EQ(framed[7], 0x04);
    EXPECT_EQ(viscaIpUnframe(framed), visca);
}

TEST(ViscaIpFraming, BareViscaIsLeftAlone) {
    const std::vector<uint8_t> bare = {0x90, 0x50, 0xFF};
    EXPECT_EQ(viscaIpUnframe(bare), bare);
    EXPECT_EQ(viscaIpUnframe({0x01, 0x00}), (std::vector<uint8_t>{0x01, 0x00}));
}
