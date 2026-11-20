#include <gtest/gtest.h>

#include "OnvifPassword.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <wincrypt.h>

#include <string>
#include <vector>

using cameras::onvif::kDpapiPasswordPrefix;
using cameras::onvif::protectPassword;
using cameras::onvif::unprotectPassword;

namespace {

    // Decode the stored blob, flip one byte in the middle, and encode it again.
    // Flipping the last base64 character often changes only padding or fill bits.
    std::string tamperMiddleByte(const std::string& stored) {
        const std::string prefix = kDpapiPasswordPrefix;
        const std::string payload = stored.substr(prefix.size());
        DWORD size = 0;
        if (!CryptStringToBinaryA(payload.c_str(), static_cast<DWORD>(payload.size()), CRYPT_STRING_BASE64, nullptr,
                                  &size, nullptr, nullptr)
            || size < 2) {
            return {};
        }
        std::vector<BYTE> bytes(size);
        if (!CryptStringToBinaryA(payload.c_str(), static_cast<DWORD>(payload.size()), CRYPT_STRING_BASE64,
                                  bytes.data(), &size, nullptr, nullptr)) {
            return {};
        }
        bytes.resize(size);
        bytes[size / 2] ^= 0x5A;

        DWORD chars = 0;
        if (!CryptBinaryToStringA(bytes.data(), static_cast<DWORD>(bytes.size()),
                                  CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &chars)
            || chars == 0) {
            return {};
        }
        std::string encoded(chars, '\0');
        if (!CryptBinaryToStringA(bytes.data(), static_cast<DWORD>(bytes.size()),
                                  CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, encoded.data(), &chars)) {
            return {};
        }
        while (!encoded.empty() && encoded.back() == '\0') {
            encoded.pop_back();
        }
        return prefix + encoded;
    }

} // namespace

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
    const std::string tampered = tamperMiddleByte(*stored);
    ASSERT_FALSE(tampered.empty());
    EXPECT_NE(tampered, *stored);
    EXPECT_FALSE(unprotectPassword(tampered));
}
