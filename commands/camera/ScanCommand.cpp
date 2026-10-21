#include "ScanCommand.h"
#include "../Arguments.h"
#include "../../cameras/visca/ViscaIpScanner.h"
#include "../../cameras/visca/ViscaSerialEndpoint.h"
#include "../../cameras/visca/ViscaSerialScanner.h"
#include "ViscaLabel.h"
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

        std::string describeReply(const std::vector<uint8_t>& reply,
                                  const std::optional<cameras::visca::PanTiltRaw>& position) {
            std::ostringstream ss;
            ss << std::hex << std::uppercase << std::setfill('0');
            if (position) {
                ss << "pan 0x" << std::setw(4) << position->pan
                   << " tilt 0x" << std::setw(4) << position->tilt;
            } else if (reply.size() >= 4 && (reply[1] & 0xF0) == 0x60) {
                ss << "VISCA error 0x" << std::setw(2) << static_cast<int>(reply[2]);
            } else {
                ss << "VISCA reply";
            }
            return ss.str();
        }

        // Comma-separated, for the lines under the table.
        std::string joined(const std::vector<std::string>& items) {
            std::string out;
            for (const auto& item : items) {
                out += (out.empty() ? "" : ", ") + item;
            }
            return out;
        }

        std::string withDescription(const cameras::visca::SerialPortInfo& port) {
            return port.description.empty() ? port.port : port.port + " (" + port.description + ")";
        }

        void scanIp(data::Context& ctx, const std::vector<std::string>& tokens) {
            const std::string usage = "Usage: camera scan visca-ip [subnet] [port]";
            if (tokens.size() > 3) {
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
            std::vector<data::Context::ScannedCamera> rows;
            for (const auto& hit : hits) {
                cameras::visca::ViscaIpEndpoint ep;
                ep.host = hit.host;
                ep.port = hit.port;
                rows.emplace_back(std::move(ep));
            }
            ctx.lastScan = std::move(rows);

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
                        << std::setw(ReplyColWidth) << describeReply(hit.reply, hit.position) << " | "
                        << addedAs.value_or("-") << std::endl;
            }

            ctx.out << std::endl << "Found " << hits.size()
                    << (hits.size() == 1 ? " VISCA-IP camera." : " VISCA-IP cameras.") << std::endl;
            if (anyNew) {
                ctx.out << "Add with: camera add scanned <n...|*> [profile] (default tenveo-30x)" << std::endl;
            }
        }

        void scanSerial(data::Context& ctx, const std::vector<std::string>& tokens) {
            using Status = cameras::visca::ViscaSerialScanResult::Status;

            // Every port Windows lists, or only the ones named.
            const auto listed = cameras::visca::listSerialPorts();
            std::vector<cameras::visca::SerialPortInfo> ports;
            if (tokens.size() == 1) {
                ports = listed;
            }
            for (size_t i = 1; i < tokens.size(); ++i) {
                const auto port = cameras::visca::normalizeComPort(tokens[i]);
                if (!port) {
                    ctx.err << "Invalid port '" << tokens[i] << "'. Usage: camera scan serial [COMn...]" << std::endl;
                    return;
                }
                const auto it = std::find_if(listed.begin(), listed.end(),
                    [&port](const auto& info) { return info.port == *port; });
                ports.push_back(it != listed.end() ? *it : cameras::visca::SerialPortInfo{*port, ""});
            }

            // A port with a camera already added is held open by that camera.
            std::vector<std::string> alreadyAdded;
            std::vector<cameras::visca::SerialPortInfo> toScan;
            for (const auto& port : ports) {
                if (const auto addedAs = viscaSerialAddedAs(ctx, port.port)) {
                    alreadyAdded.push_back(port.port + " as " + *addedAs);
                } else if (std::none_of(toScan.begin(), toScan.end(),
                                        [&port](const auto& p) { return p.port == port.port; })) {
                    toScan.push_back(port);
                }
            }

            ctx.lastScan = std::vector<data::Context::ScannedCamera>{};
            if (toScan.empty()) {
                ctx.out << (ports.empty() ? "No COM ports found. Is the USB serial adapter plugged in?"
                                          : "Nothing to scan.");
                ctx.out << std::endl;
                if (!alreadyAdded.empty()) {
                    ctx.out << "Already added, not scanned: " << joined(alreadyAdded) << "." << std::endl;
                }
                return;
            }

            std::vector<std::string> names;
            for (const auto& port : toScan) {
                names.push_back(port.port);
            }
            std::string rates;
            const auto& bauds = cameras::visca::serialScanBauds();
            for (size_t i = 0; i < bauds.size(); ++i) {
                rates += (i == 0 ? "" : (i + 1 == bauds.size() ? " and " : ", ")) + std::to_string(bauds[i]);
            }
            ctx.out << "Scanning " << joined(names) << " at " << rates << " baud..." << std::endl;

            const auto results = cameras::visca::scanViscaSerial(toScan);

            std::vector<data::Context::ScannedCamera> rows;
            std::vector<const cameras::visca::ViscaSerialScanResult*> hits;
            std::vector<std::string> silent, busy, failed;
            for (const auto& result : results) {
                switch (result.status) {
                case Status::Answered: {
                    cameras::visca::ViscaSerialEndpoint ep;
                    ep.port = result.port.port;
                    ep.baud = result.baud;
                    ep.address = result.address;
                    rows.emplace_back(std::move(ep));
                    hits.push_back(&result);
                    break;
                }
                case Status::Silent:
                    silent.push_back(withDescription(result.port));
                    break;
                case Status::Busy:
                    busy.push_back(withDescription(result.port));
                    break;
                case Status::Failed:
                    // ERROR_FILE_NOT_FOUND: no such port, e.g. an unplugged adapter.
                    failed.push_back(result.port.port + (result.openError == 2
                        ? " (not found)" : " (error " + std::to_string(result.openError) + ")"));
                    break;
                }
            }
            ctx.lastScan = std::move(rows);

            if (hits.empty()) {
                ctx.out << "No serial cameras answered." << std::endl;
            } else {
                const int IndexColWidth = 3;
                const int PortColWidth = 6;
                const int BaudColWidth = 6;
                const int AddressColWidth = 7;
                const int ReplyColWidth = 24;
                ctx.out << std::endl << std::left
                        << std::setw(IndexColWidth) << "#" << " | "
                        << std::setw(PortColWidth) << "Port" << " | "
                        << std::setw(BaudColWidth) << "Baud" << " | "
                        << std::setw(AddressColWidth) << "Address" << " | "
                        << std::setw(ReplyColWidth) << "Reply" << " | " << "Adapter" << std::endl
                        << std::string(IndexColWidth, '-') << "-+-"
                        << std::string(PortColWidth, '-') << "-+-"
                        << std::string(BaudColWidth, '-') << "-+-"
                        << std::string(AddressColWidth, '-') << "-+-"
                        << std::string(ReplyColWidth, '-') << "-+-" << std::string(20, '-') << std::endl;
                for (size_t i = 0; i < hits.size(); ++i) {
                    const auto& hit = *hits[i];
                    ctx.out << std::left
                            << std::setw(IndexColWidth) << (i + 1) << " | "
                            << std::setw(PortColWidth) << hit.port.port << " | "
                            << std::setw(BaudColWidth) << hit.baud << " | "
                            << std::setw(AddressColWidth) << hit.address << " | "
                            << std::setw(ReplyColWidth) << describeReply(hit.reply, hit.position) << " | "
                            << (hit.port.description.empty() ? "-" : hit.port.description) << std::endl;
                }
                ctx.out << std::endl << "Found " << hits.size()
                        << (hits.size() == 1 ? " serial camera." : " serial cameras.") << std::endl;
            }

            if (!silent.empty()) {
                ctx.out << "No reply: " << joined(silent) << "." << std::endl;
            }
            if (!busy.empty()) {
                ctx.out << "In use by another program: " << joined(busy) << "." << std::endl;
            }
            if (!failed.empty()) {
                ctx.out << "Could not open: " << joined(failed) << "." << std::endl;
            }
            if (!alreadyAdded.empty()) {
                ctx.out << "Already added, not scanned: " << joined(alreadyAdded) << "." << std::endl;
            }
            if (!hits.empty()) {
                ctx.out << "Add with: camera add scanned <n...|*> [profile] (default "
                        << cameras::visca::kSerialDefaultProfile << ")" << std::endl;
            }
        }
    }

    void ScanCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (!tokens.empty() && tokens[0] == "visca-ip") {
            scanIp(ctx, tokens);
        } else if (!tokens.empty() && tokens[0] == "serial") {
            scanSerial(ctx, tokens);
        } else {
            ctx.err << "Usage: camera scan visca-ip [subnet] [port]" << std::endl
                    << "       camera scan serial [COMn...]" << std::endl;
        }
    }
}
