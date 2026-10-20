#include "ScanCommand.h"
#include "../Arguments.h"
#include "../../cameras/visca/ViscaIpScanner.h"
#include "ViscaIpLabel.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace commands::camera {
    namespace {
        bool isNumber(const std::string& token) {
            return !token.empty() && token.size() <= 5 &&
                   std::all_of(token.begin(), token.end(), [](char c) { return c >= '0' && c <= '9'; });
        }

        std::string describeReply(const cameras::visca::ViscaIpScanHit& hit) {
            std::ostringstream ss;
            ss << std::hex << std::uppercase << std::setfill('0');
            if (hit.position) {
                ss << "pan 0x" << std::setw(4) << hit.position->pan
                   << " tilt 0x" << std::setw(4) << hit.position->tilt;
            } else if (hit.reply.size() >= 4 && (hit.reply[1] & 0xF0) == 0x60) {
                ss << "VISCA error 0x" << std::setw(2) << static_cast<int>(hit.reply[2]);
            } else {
                ss << "VISCA reply";
            }
            return ss.str();
        }
    }

    void ScanCommand::execute(data::Context& ctx, const std::string& args) {
        const std::string usage = "Usage: camera scan visca-ip [subnet] [port]";
        auto tokens = commands::splitArgs(args);
        if (tokens.empty() || tokens[0] != "visca-ip" || tokens.size() > 3) {
            ctx.err << usage << std::endl;
            return;
        }

        // Subnet and port may come in either order: a number is the port,
        // anything with a dot is the subnet.
        std::optional<cameras::visca::Ipv4Network> subnet;
        int port = 52381;
        bool portGiven = false;
        for (size_t i = 1; i < tokens.size(); ++i) {
            const auto& token = tokens[i];
            if (isNumber(token) && !portGiven) {
                port = std::stoi(token);
                if (port <= 0 || port > 65535) {
                    ctx.err << "Invalid port." << std::endl;
                    return;
                }
                portGiven = true;
            } else if (token.find('.') != std::string::npos && !subnet) {
                std::string error;
                subnet = cameras::visca::parseIpv4Network(token, error);
                if (!subnet) {
                    ctx.err << "Invalid subnet: " << error << std::endl;
                    return;
                }
            } else {
                ctx.err << usage << std::endl;
                return;
            }
        }

        const auto networks = subnet ? std::vector<cameras::visca::Ipv4Network>{*subnet}
                                     : cameras::visca::localIpv4Networks();
        if (networks.empty()) {
            ctx.err << "No local IPv4 network found to scan. Give a subnet, e.g. "
                       "camera scan visca-ip 192.168.1.0/24" << std::endl;
            return;
        }

        std::string names;
        size_t addresses = 0;
        for (const auto& network : networks) {
            names += (names.empty() ? "" : ", ") + network.toString();
            addresses += network.hosts().size();
        }
        ctx.out << "Scanning " << names << " on port " << port << " (" << addresses
                << (addresses == 1 ? " address" : " addresses") << ")..." << std::endl;

        std::string error;
        const auto hits = cameras::visca::scanViscaIp(networks, port, error);
        if (!error.empty()) {
            ctx.err << "Scan failed: " << error << std::endl;
            return;
        }

        // Row n of the table is entry n-1, for `camera add scanned <n>`.
        std::vector<cameras::visca::ViscaIpEndpoint> rows;
        for (const auto& hit : hits) {
            cameras::visca::ViscaIpEndpoint ep;
            ep.host = hit.host;
            ep.port = hit.port;
            rows.push_back(std::move(ep));
        }
        ctx.lastViscaIpScan = std::move(rows);

        if (hits.empty()) {
            ctx.out << "No VISCA-IP cameras answered." << std::endl;
            return;
        }

        const int IndexColWidth = 3;
        const int HostColWidth = 21;
        const int ReplyColWidth = 24;
        ctx.out << std::endl << std::left
                << std::setw(IndexColWidth) << "#" << " | "
                << std::setw(HostColWidth) << "Address" << " | "
                << std::setw(ReplyColWidth) << "Reply" << " | " << "Added as" << std::endl
                << std::string(IndexColWidth, '-') << "-+-"
                << std::string(HostColWidth, '-') << "-+-"
                << std::string(ReplyColWidth, '-') << "-+-" << std::string(20, '-') << std::endl;

        bool anyNew = false;
        for (size_t i = 0; i < hits.size(); ++i) {
            const auto& hit = hits[i];
            const auto addedAs = viscaIpAddedAs(ctx, hit.host, hit.port);
            anyNew = anyNew || !addedAs;
            ctx.out << std::left
                    << std::setw(IndexColWidth) << (i + 1) << " | "
                    << std::setw(HostColWidth) << (hit.host + ":" + std::to_string(hit.port)) << " | "
                    << std::setw(ReplyColWidth) << describeReply(hit) << " | "
                    << addedAs.value_or("-") << std::endl;
        }

        ctx.out << std::endl << "Found " << hits.size()
                << (hits.size() == 1 ? " VISCA-IP camera." : " VISCA-IP cameras.") << std::endl;
        if (anyNew) {
            ctx.out << "Add with: camera add scanned <n...|*> [profile]" << std::endl;
        }
    }
}
