#include "VersionCommand.h"

#include "../Arguments.h"
#include "ptz_version.h"

#include <cctype>
#include <nlohmann/json.hpp>
#include <string>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winhttp.h>
#endif

namespace commands::console {

    namespace {

        constexpr size_t kMaxReleaseBody = 256 * 1024;

        bool parseReleaseTag(const std::string& tag, int& major, int& minor, int& patch) {
            std::string text = tag;
            if (!text.empty() && (text[0] == 'v' || text[0] == 'V')) {
                text.erase(text.begin());
            }

            int parts[3] = {0, 0, 0};
            size_t index = 0;
            for (int part = 0; part < 3; ++part) {
                if (index >= text.size() ||
                    !std::isdigit(static_cast<unsigned char>(text[index]))) {
                    return false;
                }
                int value = 0;
                while (index < text.size() &&
                       std::isdigit(static_cast<unsigned char>(text[index]))) {
                    value = value * 10 + (text[index] - '0');
                    if (value > 65535) {
                        return false;
                    }
                    ++index;
                }
                parts[part] = value;
                if (part < 2) {
                    if (index >= text.size() || text[index] != '.') {
                        return false;
                    }
                    ++index;
                }
            }
            if (index != text.size()) {
                return false;
            }
            major = parts[0];
            minor = parts[1];
            patch = parts[2];
            return true;
        }

        int compareVersions(int majorA, int minorA, int patchA,
                            int majorB, int minorB, int patchB) {
            if (majorA != majorB) {
                return majorA < majorB ? -1 : 1;
            }
            if (minorA != minorB) {
                return minorA < minorB ? -1 : 1;
            }
            if (patchA != patchB) {
                return patchA < patchB ? -1 : 1;
            }
            return 0;
        }

        void printVersion(data::Context& ctx) {
            ctx.out << "PTZ Commander " << PTZ_VERSION_STRING << std::endl;
        }

        void printDevBuild(data::Context& ctx) {
#if !PTZ_VERSION_HAS_SEMVER
            ctx.out << "This build is not from a release tag." << std::endl;
#elif !PTZ_VERSION_IS_RELEASE
            const std::string base = std::to_string(PTZ_VERSION_MAJOR) + "." +
                                     std::to_string(PTZ_VERSION_MINOR) + "." +
                                     std::to_string(PTZ_VERSION_PATCH);
            if (PTZ_VERSION_COMMITS > 0) {
                ctx.out << "This build is " << PTZ_VERSION_COMMITS
                        << (PTZ_VERSION_COMMITS == 1 ? " commit" : " commits")
                        << " after " << base;
                if (PTZ_VERSION_DIRTY) {
                    ctx.out << " and has uncommitted changes";
                }
                ctx.out << "." << std::endl;
            }
            else if (PTZ_VERSION_DIRTY) {
                ctx.out << "This build has uncommitted changes on " << base << "." << std::endl;
            }
#endif
        }

#ifdef _WIN32
        struct WinHttpHandle {
            HINTERNET handle = nullptr;
            explicit WinHttpHandle(HINTERNET h) : handle(h) {}
            ~WinHttpHandle() {
                if (handle) {
                    WinHttpCloseHandle(handle);
                }
            }
            WinHttpHandle(const WinHttpHandle&) = delete;
            WinHttpHandle& operator=(const WinHttpHandle&) = delete;
        };

        std::string winhttpFailure(const char* what) {
            const DWORD code = GetLastError();
            if (code == ERROR_WINHTTP_TIMEOUT) {
                return "timed out contacting GitHub";
            }
            if (code == ERROR_WINHTTP_NAME_NOT_RESOLVED) {
                return "could not resolve api.github.com";
            }
            if (code == ERROR_WINHTTP_CANNOT_CONNECT) {
                return "could not connect to api.github.com";
            }
            return std::string(what) + " (" + std::to_string(code) + ")";
        }

        bool fetchLatestRelease(std::string& tag, std::string& url, std::string& error) {
#if defined(WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY)
            const DWORD accessType = WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY;
#else
            const DWORD accessType = WINHTTP_ACCESS_TYPE_DEFAULT_PROXY;
#endif
            WinHttpHandle session(WinHttpOpen(L"PTZCommander", accessType,
                                               WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
            if (!session.handle) {
                error = winhttpFailure("could not open an HTTP session");
                return false;
            }

            WinHttpSetTimeouts(session.handle, 10000, 10000, 10000, 10000);

            WinHttpHandle connection(WinHttpConnect(session.handle, L"api.github.com",
                                                    INTERNET_DEFAULT_HTTPS_PORT, 0));
            if (!connection.handle) {
                error = winhttpFailure("could not connect to api.github.com");
                return false;
            }

            WinHttpHandle request(WinHttpOpenRequest(
                connection.handle, L"GET",
                L"/repos/ciroque/ptz-commander/releases/latest",
                nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                WINHTTP_FLAG_SECURE));
            if (!request.handle) {
                error = winhttpFailure("could not create the GitHub request");
                return false;
            }

            const wchar_t* headers =
                L"Accept: application/vnd.github+json\r\nUser-Agent: PTZCommander\r\n";
            if (!WinHttpSendRequest(request.handle, headers, static_cast<DWORD>(-1),
                                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
                !WinHttpReceiveResponse(request.handle, nullptr)) {
                error = winhttpFailure("the request to GitHub failed");
                return false;
            }

            DWORD status = 0;
            DWORD statusSize = sizeof(status);
            if (!WinHttpQueryHeaders(request.handle,
                                     WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                     WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                                     WINHTTP_NO_HEADER_INDEX)) {
                error = winhttpFailure("could not read the GitHub response status");
                return false;
            }
            if (status == 404) {
                error = "no GitHub release was found";
                return false;
            }
            if (status == 403) {
                error = "GitHub refused the request";
                return false;
            }
            if (status != 200) {
                error = "GitHub returned HTTP " + std::to_string(status);
                return false;
            }

            std::string body;
            body.reserve(4096);
            for (;;) {
                DWORD available = 0;
                if (!WinHttpQueryDataAvailable(request.handle, &available)) {
                    error = winhttpFailure("could not read the GitHub response");
                    return false;
                }
                if (available == 0) {
                    break;
                }
                if (body.size() + available > kMaxReleaseBody) {
                    error = "the GitHub response was too large";
                    return false;
                }
                const size_t offset = body.size();
                body.resize(offset + available);
                DWORD read = 0;
                if (!WinHttpReadData(request.handle, body.data() + offset, available, &read)) {
                    error = winhttpFailure("could not read the GitHub response");
                    return false;
                }
                body.resize(offset + read);
                if (read == 0) {
                    break;
                }
            }

            nlohmann::json payload;
            try {
                payload = nlohmann::json::parse(body);
            }
            catch (const nlohmann::json::exception&) {
                error = "the GitHub response was not a release description";
                return false;
            }
            if (!payload.is_object() || !payload.contains("tag_name") ||
                !payload["tag_name"].is_string()) {
                error = "the GitHub response did not include a release tag";
                return false;
            }
            tag = payload["tag_name"].get<std::string>();
            if (payload.contains("html_url") && payload["html_url"].is_string()) {
                url = payload["html_url"].get<std::string>();
            }
            return true;
        }
#else
        bool fetchLatestRelease(std::string&, std::string&, std::string& error) {
            error = "update check is only available on Windows";
            return false;
        }
#endif

        void checkForUpdate(data::Context& ctx) {
            printDevBuild(ctx);

            std::string tag;
            std::string url;
            std::string error;
            if (!fetchLatestRelease(tag, url, error)) {
                ctx.err << "Could not check for updates: " << error << std::endl;
                return;
            }

            int major = 0;
            int minor = 0;
            int patch = 0;
            if (!parseReleaseTag(tag, major, minor, patch)) {
                ctx.err << "Could not check for updates: latest release tag '" << tag
                        << "' is not a version." << std::endl;
                return;
            }

            ctx.out << "Latest release: " << major << "." << minor << "." << patch << std::endl;

#if PTZ_VERSION_HAS_SEMVER
            const int cmp = compareVersions(PTZ_VERSION_MAJOR, PTZ_VERSION_MINOR, PTZ_VERSION_PATCH,
                                            major, minor, patch);
            if (cmp < 0) {
                ctx.out << "A newer release is available";
                if (!url.empty()) {
                    ctx.out << ": " << url;
                }
                ctx.out << std::endl;
            }
            else if (cmp > 0) {
                ctx.out << "This build is newer than the latest release." << std::endl;
            }
            else if (PTZ_VERSION_IS_RELEASE) {
                ctx.out << "This install is current." << std::endl;
            }
#else
            if (!url.empty()) {
                ctx.out << "Release page: " << url << std::endl;
            }
#endif
        }

    } // namespace

    void VersionCommand::execute(data::Context& ctx, const std::string& args) {
        const auto tokens = commands::splitArgs(args);
        if (tokens.empty()) {
            printVersion(ctx);
            return;
        }
        if (tokens.size() == 1 && tokens[0] == "check") {
            printVersion(ctx);
            checkForUpdate(ctx);
            return;
        }
        ctx.err << "Usage: version [check]" << std::endl;
    }

} // namespace commands::console
