#include "AddCommand.h"
#include "../Arguments.h"
#include "../../cameras/visca/ViscaIpEndpoint.h"
#include "../../cameras/visca/ViscaProfile.h"
#include "../../cameras/visca/ViscaSerialEndpoint.h"
#include "ViscaLabel.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <variant>

namespace commands::camera {
    namespace {
        const char* kDefaultProfile = "tenveo-30x";

        bool knownProfile(data::Context& ctx, const std::string& profile) {
            if (cameras::visca::profileByName(profile)) {
                return true;
            }
            ctx.err << "Unknown profile '" << profile << "'. Known: "
                    << cameras::visca::knownProfileNames() << std::endl;
            return false;
        }

        // camera add scanned <n...|*> [profile]: rows of the last scan.
        void addScanned(data::Context& ctx, const std::vector<std::string>& tokens) {
            const std::string usage = "Usage: camera add scanned <n...|*> [profile]";
            if (!ctx.lastScan) {
                ctx.err << "Run `camera scan visca-ip` or `camera scan serial` first." << std::endl;
                return;
            }
            const auto& rows = *ctx.lastScan;

            std::set<size_t> indexes;  // 1-based, in table order
            bool all = false;
            std::string profile;
            for (size_t i = 1; i < tokens.size(); ++i) {
                const auto& token = tokens[i];
                if (token == "*") {
                    all = true;
                } else if (!token.empty() && token.size() <= 4 &&
                           std::all_of(token.begin(), token.end(), [](char c) { return c >= '0' && c <= '9'; })) {
                    indexes.insert(static_cast<size_t>(std::stoul(token)));
                } else if (profile.empty()) {
                    profile = token;
                } else {
                    ctx.err << usage << std::endl;
                    return;
                }
            }
            if (!all && indexes.empty()) {
                ctx.err << usage << std::endl;
                return;
            }
            if (rows.empty()) {
                ctx.err << "The last scan found no cameras." << std::endl;
                return;
            }
            // Check every index before adding any, so a typo adds nothing.
            for (const size_t n : indexes) {
                if (n < 1 || n > rows.size()) {
                    ctx.err << "No row " << n << "; the last scan found " << rows.size()
                            << (rows.size() == 1 ? " camera." : " cameras.") << std::endl;
                    return;
                }
            }
            // Without one, each row gets its kind's default profile.
            if (!profile.empty() && !knownProfile(ctx, profile)) {
                return;
            }
            if (all) {
                for (size_t n = 1; n <= rows.size(); ++n) {
                    indexes.insert(n);
                }
            }

            bool addedAny = false;
            bool addedIpWithDefault = false;
            for (const size_t n : indexes) {
                std::string error;
                if (const auto* row = std::get_if<cameras::visca::ViscaIpEndpoint>(&rows[n - 1])) {
                    auto ep = *row;
                    if (const auto addedAs = viscaIpAddedAs(ctx, ep.host, ep.port)) {
                        ctx.out << "#" << n << " " << ep.host << ":" << ep.port
                                << " is already added as " << *addedAs << std::endl;
                        continue;
                    }
                    ep.profile = profile.empty() ? kDefaultProfile : profile;
                    if (!ctx.addViscaIp || !ctx.addViscaIp(ep, error)) {
                        ctx.err << "Could not add #" << n << " " << ep.host << ":" << ep.port << ": "
                                << (ctx.addViscaIp ? error : "VISCA-IP is not available") << std::endl;
                        continue;
                    }
                    ctx.out << "Added #" << n << " " << viscaIpSerial(ep.host, ep.port)
                            << " profile=" << ep.profile << std::endl;
                    addedIpWithDefault = addedIpWithDefault || profile.empty();
                } else {
                    auto ep = std::get<cameras::visca::ViscaSerialEndpoint>(rows[n - 1]);
                    if (const auto addedAs = viscaSerialAddedAs(ctx, ep.port)) {
                        ctx.out << "#" << n << " " << ep.port << " is already added as " << *addedAs << std::endl;
                        continue;
                    }
                    ep.profile = profile.empty() ? cameras::visca::kSerialDefaultProfile : profile;
                    if (!ctx.addViscaSerial || !ctx.addViscaSerial(ep, error)) {
                        ctx.err << "Could not add #" << n << " " << ep.port << ": "
                                << (ctx.addViscaSerial ? error : "VISCA serial is not available") << std::endl;
                        continue;
                    }
                    ctx.out << "Added #" << n << " " << cameras::visca::serialCameraId(ep) << " baud=" << ep.baud
                            << " profile=" << ep.profile << std::endl;
                }
                addedAny = true;
            }

            if (addedAny) {
                ctx.notifySetupChanged();
                if (addedIpWithDefault) {
                    // A scan can't tell camera models apart.
                    ctx.out << "If a camera isn't a Tenveo 30x, run `camera calibrate <id>` to measure its ranges."
                            << std::endl;
                }
            }
        }

        bool isDigits(const std::string& token) {
            return !token.empty() && std::all_of(token.begin(), token.end(), [](char c) { return c >= '0' && c <= '9'; });
        }

        // camera add serial <COMn> [baud] [address] [profile]
        void addSerial(data::Context& ctx, const std::vector<std::string>& tokens) {
            const std::string usage = "Usage: camera add serial <COMn> [baud] [address] [profile]";
            if (tokens.size() < 2 || tokens.size() > 5) {
                ctx.err << usage << std::endl;
                return;
            }
            const auto port = cameras::visca::normalizeComPort(tokens[1]);
            if (!port) {
                ctx.err << "Invalid port '" << tokens[1] << "'; give a COM port such as COM3." << std::endl;
                return;
            }
            cameras::visca::ViscaSerialEndpoint ep;
            ep.port = *port;

            // Numbers are the baud rate, then the address; a word is the profile.
            int numbers = 0;
            bool profileGiven = false;
            for (size_t i = 2; i < tokens.size(); ++i) {
                const auto& token = tokens[i];
                if (isDigits(token) && token.size() <= 6 && numbers < 2 && !profileGiven) {
                    const int value = std::stoi(token);
                    if (numbers == 0 && !cameras::visca::validSerialBaud(value)) {
                        ctx.err << "Invalid baud rate " << token << "; use one of "
                                << cameras::visca::validSerialBauds() << "." << std::endl;
                        return;
                    }
                    if (numbers == 1 && (value < 1 || value > 7)) {
                        ctx.err << "Invalid address " << token << "; VISCA camera addresses are 1-7." << std::endl;
                        return;
                    }
                    (numbers == 0 ? ep.baud : ep.address) = value;
                    ++numbers;
                } else if (!profileGiven && !isDigits(token)) {
                    ep.profile = token;
                    profileGiven = true;
                } else {
                    ctx.err << usage << std::endl;
                    return;
                }
            }
            if (!knownProfile(ctx, ep.profile)) {
                return;
            }

            std::string error;
            if (!ctx.addViscaSerial(ep, error)) {
                ctx.err << "Could not add serial camera " << cameras::visca::serialCameraId(ep) << ": " << error
                        << std::endl;
                return;
            }
            ctx.notifySetupChanged();
            ctx.out << "Added " << cameras::visca::serialCameraId(ep) << " baud=" << ep.baud
                    << " profile=" << ep.profile << std::endl;
        }
    }

    void AddCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (!tokens.empty() && tokens[0] == "serial") {
            if (!ctx.addViscaSerial) {
                ctx.err << "VISCA serial is not available." << std::endl;
                return;
            }
            addSerial(ctx, tokens);
            return;
        }
        if (!tokens.empty() && tokens[0] == "scanned") {
            addScanned(ctx, tokens);
            return;
        }
        if (!ctx.addViscaIp && !tokens.empty() && tokens[0] == "visca-ip") {
            ctx.err << "VISCA-IP is not available." << std::endl;
            return;
        }
        if (tokens.size() < 2 || tokens[0] != "visca-ip") {
            ctx.err << "Usage: camera add visca-ip <host> [port] [profile]" << std::endl
                    << "       camera add serial <COMn> [baud] [address] [profile]" << std::endl
                    << "       camera add scanned <n...|*> [profile]" << std::endl;
            return;
        }

        cameras::visca::ViscaIpEndpoint ep;
        ep.host = tokens[1];
        ep.port = 52381;
        ep.profile = kDefaultProfile;

        if (tokens.size() >= 3) {
            try {
                ep.port = std::stoi(tokens[2]);
            }
            catch (const std::exception&) {
                ctx.err << "Invalid port." << std::endl;
                return;
            }
            if (ep.port <= 0 || ep.port > 65535) {
                ctx.err << "Invalid port." << std::endl;
                return;
            }
        }
        if (tokens.size() >= 4) {
            ep.profile = tokens[3];
        }
        if (!knownProfile(ctx, ep.profile)) {
            return;
        }

        std::string error;
        if (!ctx.addViscaIp(ep, error)) {
            ctx.err << "Could not add VISCA-IP camera " << ep.host << ":" << ep.port << ": " << error << std::endl;
            return;
        }

        ctx.notifySetupChanged();
        ctx.out << "Added VISCA-IP:" << ep.host << ":" << ep.port
                << " profile=" << ep.profile << std::endl;
    }
}
