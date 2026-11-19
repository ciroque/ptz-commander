#ifndef CAMERAS_ONVIF_ONVIF_PASSWORD_H
#define CAMERAS_ONVIF_ONVIF_PASSWORD_H

#include <optional>
#include <string>

namespace cameras::onvif {

    // Stored form is "dpapi:" plus the base64 of a CryptProtectData blob.
    // A value without that prefix is a legacy plaintext password.
    inline constexpr const char* kDpapiPasswordPrefix = "dpapi:";

    // Encrypts for the current Windows user. nullopt if DPAPI refuses.
    std::optional<std::string> protectPassword(const std::string& password);

    // Decrypts a "dpapi:" value. A value without the prefix is returned as
    // plaintext so older .ptzc files still load. nullopt if a prefixed
    // value cannot be decoded or decrypted on this Windows account.
    std::optional<std::string> unprotectPassword(const std::string& stored);

} // namespace cameras::onvif

#endif
