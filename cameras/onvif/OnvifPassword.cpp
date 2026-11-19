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

namespace cameras::onvif {

    namespace {

        std::optional<std::string> base64Encode(const BYTE* data, DWORD size) {
            if (size == 0) {
                return std::string();
            }
            DWORD chars = 0;
            if (!CryptBinaryToStringA(data, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, nullptr, &chars) || chars == 0) {
                return std::nullopt;
            }
            std::string out(chars, '\0');
            if (!CryptBinaryToStringA(data, size, CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, out.data(), &chars)) {
                return std::nullopt;
            }
            while (!out.empty() && out.back() == '\0') {
                out.pop_back();
            }
            return out;
        }

        std::optional<std::vector<BYTE>> base64Decode(const std::string& text) {
            if (text.empty()) {
                return std::vector<BYTE>();
            }
            DWORD size = 0;
            if (!CryptStringToBinaryA(text.c_str(), static_cast<DWORD>(text.size()), CRYPT_STRING_BASE64, nullptr, &size, nullptr, nullptr)) {
                return std::nullopt;
            }
            std::vector<BYTE> out(size);
            if (size > 0 && !CryptStringToBinaryA(text.c_str(), static_cast<DWORD>(text.size()), CRYPT_STRING_BASE64, out.data(), &size, nullptr, nullptr)) {
                return std::nullopt;
            }
            out.resize(size);
            return out;
        }

    } // namespace

    std::optional<std::string> protectPassword(const std::string& password) {
        DATA_BLOB input{};
        input.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(password.data()));
        input.cbData = static_cast<DWORD>(password.size());

        DATA_BLOB output{};
        const BOOL ok = CryptProtectData(&input, L"PTZ Commander ONVIF password", nullptr, nullptr, nullptr,
                                         CRYPTPROTECT_UI_FORBIDDEN, &output);
        if (!ok || output.pbData == nullptr) {
            return std::nullopt;
        }

        auto encoded = base64Encode(output.pbData, output.cbData);
        LocalFree(output.pbData);
        if (!encoded) {
            return std::nullopt;
        }
        return std::string(kDpapiPasswordPrefix) + *encoded;
    }

    std::optional<std::string> unprotectPassword(const std::string& stored) {
        const std::string prefix = kDpapiPasswordPrefix;
        if (stored.compare(0, prefix.size(), prefix) != 0) {
            return stored;
        }

        const auto blob = base64Decode(stored.substr(prefix.size()));
        if (!blob) {
            return std::nullopt;
        }

        DATA_BLOB input{};
        input.pbData = const_cast<BYTE*>(blob->data());
        input.cbData = static_cast<DWORD>(blob->size());

        DATA_BLOB output{};
        const BOOL ok = CryptUnprotectData(&input, nullptr, nullptr, nullptr, nullptr, CRYPTPROTECT_UI_FORBIDDEN, &output);
        if (!ok) {
            return std::nullopt;
        }

        std::string password(reinterpret_cast<const char*>(output.pbData), output.cbData);
        LocalFree(output.pbData);
        return password;
    }

} // namespace cameras::onvif
