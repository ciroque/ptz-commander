#include "AddIpCommand.h"
#include "../Arguments.h"
#include "../../cameras/visca/ViscaIpEndpoint.h"
#include "../../cameras/visca/ViscaProfile.h"
#include "ViscaIpLabel.h"
#include <algorithm>
#include <iostream>
#include <set>
#include <string>

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
            if (!ctx.lastViscaIpScan) {
                ctx.err << "Run `camera scan visca-ip` first." << std::endl;
                return;
            }
            const auto& rows = *ctx.lastViscaIpScan;

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
            if (profile.empty()) {
                profile = kDefaultProfile;
            }
            if (!knownProfile(ctx, profile)) {
                return;
            }
            if (all) {
                for (size_t n = 1; n <= rows.size(); ++n) {
                    indexes.insert(n);
                }
            }

            bool addedAny = false;
            for (const size_t n : indexes) {
                auto ep = rows[n - 1];
                if (const auto addedAs = viscaIpAddedAs(ctx, ep.host, ep.port)) {
                    ctx.out << "#" << n << " " << ep.host << ":" << ep.port
                            << " is already added as " << *addedAs << std::endl;
                    continue;
                }
                ep.profile = profile;
                if (!ctx.addViscaIp(ep)) {
                    ctx.err << "Could not add #" << n << " " << ep.host << ":" << ep.port << std::endl;
                    continue;
                }
                addedAny = true;
                ctx.out << "Added #" << n << " " << viscaIpSerial(ep.host, ep.port)
                        << " profile=" << profile << std::endl;
            }

            if (addedAny) {
                ctx.notifySetupChanged();
                if (profile == kDefaultProfile) {
                    // A scan can't tell camera models apart.
                    ctx.out << "If a camera isn't a Tenveo 30x, run `camera calibrate <id>` to measure its ranges."
                            << std::endl;
                }
            }
        }
    }

    void AddIpCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (!ctx.addViscaIp && !tokens.empty() && (tokens[0] == "visca-ip" || tokens[0] == "scanned")) {
            ctx.err << "VISCA-IP is not available." << std::endl;
            return;
        }
        if (!tokens.empty() && tokens[0] == "scanned") {
            addScanned(ctx, tokens);
            return;
        }
        if (tokens.size() < 2 || tokens[0] != "visca-ip") {
            ctx.err << "Usage: camera add visca-ip <host> [port] [profile]" << std::endl
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

        if (!ctx.addViscaIp(ep)) {
            ctx.err << "Could not add VISCA-IP camera " << ep.host << std::endl;
            return;
        }

        ctx.notifySetupChanged();
        ctx.out << "Added VISCA-IP:" << ep.host << ":" << ep.port
                << " profile=" << ep.profile << std::endl;
    }
}
