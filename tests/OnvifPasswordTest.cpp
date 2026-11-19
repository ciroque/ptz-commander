#include <gtest/gtest.h>

#include "OnvifPassword.h"

using cameras::onvif::kDpapiPasswordPrefix;
using cameras::onvif::protectPassword;
using cameras::onvif::unprotectPassword;

TEST(OnvifPassword, RoundTripsAndHidesThePlaintext) {
    const auto stored = protectPassword("s3cret!");
    ASSERT_TRUE(stored);
    EXPECT_EQ(stored->compare(0, std::char_traits<char>::length(kDpapiPasswordPrefix), kDpapiPasswordPrefix), 0);
    EXPECT_EQ(stored->find("s3cret!"), std::string::npos);

    const auto back = unprotectPassword(*stored);
    ASSERT_TRUE(back);
    EXPECT_EQ(*back, "s3cret!");
}

TEST(OnvifPassword, EmptyPasswordRoundTrips) {
    const auto stored = protectPassword("");
    ASSERT_TRUE(stored);
    const auto back = unprotectPassword(*stored);
    ASSERT_TRUE(back);
    EXPECT_TRUE(back->empty());
}

TEST(OnvifPassword, LegacyPlaintextLoadsUnchanged) {
    const auto back = unprotectPassword("still-plain");
    ASSERT_TRUE(back);
    EXPECT_EQ(*back, "still-plain");
}

TEST(OnvifPassword, RejectsATamperedBlob) {
    const auto stored = protectPassword("s3cret!");
    ASSERT_TRUE(stored);
    std::string tampered = *stored;
    char& last = tampered.back();
    last = (last == 'A') ? 'B' : 'A';
    EXPECT_FALSE(unprotectPassword(tampered));
}
