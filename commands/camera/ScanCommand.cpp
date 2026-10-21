#include "ScanCommand.h"
#include "../Arguments.h"
#include "../../cameras/visca/ViscaIpScanner.h"
#include "../../cameras/visca/ViscaSerialEndpoint.h"
#include "../../cameras/visca/ViscaSerialScanner.h"
#include "ViscaLabel.h"
#include <algorithm>
#include <future>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

namespace commands::camera {
    namespace {
        namespace visca = cameras::visca;

        bool isNumber(const std::string& token) {
            return !token.empty() && token.size() <= 5 &&
                   std::all_of(token.begin(), token.end(), [](char c) { return c >= '0' && c <= '9'; });
        }

        std::string describeReply(const std::vector<uint8_t>& reply, const std::optional<visca::PanTiltRaw>& position) {
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

        std::string withDescription(const visca::SerialPortInfo& port) {
            return port.description.empty() ? port.port : port.port + " (" + port.description + ")";
        }

        // One table row, and what `camera add scanned` gets for it.
        struct Row {
            bool serial = false;
            std::string camera;
            std::string reply;
            std::string notes;
            bool added = false;
            data::Context::ScannedCamera scanned;
        };

        // What one kind of scan found, ready to print.
        struct Found {
            const char* kind = "";            // "VISCA-IP" or "serial"
            bool ran = false;                 // false if there was nothing to scan or it failed
            std::vector<Row> rows;
            std::vector<std::string> remarks; // lines under the table
        };

        struct IpPlan {
            std::vector<visca::Ipv4Network> networks;
            int port = 52381;

            std::string describe() const {
                std::string names;
                size_t addresses = 0;
                for (const auto& network : networks) {
                    names += (names.empty() ? "" : ", ") + network.toString();
                    addresses += network.hosts().size();
                }
                return names + " on port " + std::to_string(port) + " (" + std::to_string(addresses) +
                       (addresses == 1 ? " address)" : " addresses)");
            }
        };

        // `[subnet] [port]`, in either order: a number is the port, anything
        // with a dot is the subnet. nullopt with error set if they're wrong
        // or there's no network to scan.
        std::optional<IpPlan> planIp(const std::vector<std::string>& args, std::string& error) {
            std::optional<visca::Ipv4Network> subnet;
            IpPlan plan;
            bool portGiven = false;
            for (const auto& token : args) {
                if (isNumber(token) && !portGiven) {
                    plan.port = std::stoi(token);
                    if (plan.port <= 0 || plan.port > 65535) {
                        error = "Invalid port.";
                        return std::nullopt;
                    }
                    portGiven = true;
                } else if (token.find('.') != std::string::npos && !subnet) {
                    std::string reason;
                    subnet = visca::parseIpv4Network(token, reason);
                    if (!subnet) {
                        error = "Invalid subnet: " + reason;
                        return std::nullopt;
                    }
                } else {
                    error = "Usage: camera scan visca-ip [subnet] [port]";
                    return std::nullopt;
                }
            }
            plan.networks = subnet ? std::vector<visca::Ipv4Network>{*subnet} : visca::localIpv4Networks();
            if (plan.networks.empty()) {
                error = "No local IPv4 network found to scan. Give a subnet, e.g. camera scan visca-ip 192.168.1.0/24";
                return std::nullopt;
            }
            return plan;
        }

        struct IpOutcome {
            std::vector<visca::ViscaIpScanHit> hits;
            std::string error;
        };

        IpOutcome runIp(const IpPlan& plan) {
            IpOutcome outcome;
            outcome.hits = visca::scanViscaIp(plan.networks, plan.port, outcome.error);
            return outcome;
        }

        Found foundIp(const data::Context& ctx, const IpOutcome& outcome) {
            Found found;
            found.kind = "VISCA-IP";
            if (!outcome.error.empty()) {
                found.remarks.push_back("VISCA-IP scan failed: " + outcome.error);
                return found;
            }
            found.ran = true;
            for (const auto& hit : outcome.hits) {
                visca::ViscaIpEndpoint ep;
                ep.host = hit.host;
                ep.port = hit.port;
                const auto addedAs = viscaIpAddedAs(ctx, hit.host, hit.port);
                Row row;
                row.camera = hit.host + ":" + std::to_string(hit.port);
                row.reply = describeReply(hit.reply, hit.position);
                row.notes = addedAs ? "added as " + *addedAs : "";
                row.added = addedAs.has_value();
                row.scanned = std::move(ep);
                found.rows.push_back(std::move(row));
            }
            return found;
        }

        struct SerialPlan {
            bool anyPorts = false;
            std::vector<visca::SerialPortInfo> toScan;
            std::vector<std::string> alreadyAdded;  // "COM4 as cam1"

            std::string describe() const {
                std::vector<std::string> names;
                for (const auto& port : toScan) {
                    names.push_back(port.port);
                }
                std::string rates;
                const auto& bauds = visca::serialScanBauds();
                for (size_t i = 0; i < bauds.size(); ++i) {
                    rates += (i == 0 ? "" : (i + 1 == bauds.size() ? " and " : ", ")) + std::to_string(bauds[i]);
                }
                return joined(names) + " at " + rates + " baud";
            }
        };

        // Every port Windows lists, or only the ones named. nullopt with
        // error set for a name that isn't a COM port.
        std::optional<SerialPlan> planSerial(const data::Context& ctx, const std::vector<std::string>& args,
                                             std::string& error) {
            const auto listed = visca::listSerialPorts();
            std::vector<visca::SerialPortInfo> ports;
            if (args.empty()) {
                ports = listed;
            }
            for (const auto& token : args) {
                const auto port = visca::normalizeComPort(token);
                if (!port) {
                    error = "Invalid port '" + token + "'. Usage: camera scan serial [COMn...]";
                    return std::nullopt;
                }
                const auto it = std::find_if(listed.begin(), listed.end(),
                    [&port](const auto& info) { return info.port == *port; });
                ports.push_back(it != listed.end() ? *it : visca::SerialPortInfo{*port, ""});
            }

            // A port with a camera already added is held open by that camera.
            SerialPlan plan;
            plan.anyPorts = !ports.empty();
            for (const auto& port : ports) {
                if (const auto addedAs = viscaSerialAddedAs(ctx, port.port)) {
                    plan.alreadyAdded.push_back(port.port + " as " + *addedAs);
                } else if (std::none_of(plan.toScan.begin(), plan.toScan.end(),
                                        [&port](const auto& p) { return p.port == port.port; })) {
                    plan.toScan.push_back(port);
                }
            }
            return plan;
        }

        Found foundSerial(const SerialPlan& plan, const std::vector<visca::ViscaSerialScanResult>& results) {
            using Status = visca::ViscaSerialScanResult::Status;
            Found found;
            found.kind = "serial";
            found.ran = !plan.toScan.empty();
            if (!found.ran) {
                found.remarks.push_back(plan.anyPorts ? "No COM ports to scan."
                                                      : "No COM ports found. Is the USB serial adapter plugged in?");
            }

            std::vector<std::string> silent, busy, failed;
            for (const auto& result : results) {
                switch (result.status) {
                case Status::Answered: {
                    visca::ViscaSerialEndpoint ep;
                    ep.port = result.port.port;
                    ep.baud = result.baud;
                    ep.address = result.address;
                    Row row;
                    row.serial = true;
                    row.camera = ep.port + " @" + std::to_string(ep.baud) + " addr " + std::to_string(ep.address);
                    row.reply = describeReply(result.reply, result.position);
                    row.notes = result.port.description;
                    row.scanned = std::move(ep);
                    found.rows.push_back(std::move(row));
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
            if (!silent.empty()) {
                found.remarks.push_back("No reply on " + joined(silent) + ".");
            }
            if (!busy.empty()) {
                found.remarks.push_back("In use by another program: " + joined(busy) + ".");
            }
            if (!failed.empty()) {
                found.remarks.push_back("Could not open " + joined(failed) + ".");
            }
            if (!plan.alreadyAdded.empty()) {
                found.remarks.push_back("Already added, not scanned: " + joined(plan.alreadyAdded) + ".");
            }
            return found;
        }

        // One numbered table for all the scans that ran, then the lines
        // under it. Replaces the rows `camera add scanned` uses.
        void report(data::Context& ctx, const std::vector<Found>& scans) {
            std::vector<const Row*> rows;
            std::vector<data::Context::ScannedCamera> scanned;
            for (const auto& found : scans) {
                for (const auto& row : found.rows) {
                    rows.push_back(&row);
                    scanned.push_back(row.scanned);
                }
            }
            ctx.lastScan = std::move(scanned);

            if (rows.empty()) {
                // Name the kind unless every scan ran; say nothing if none
                // did, since the lines below explain why.
                std::vector<std::string> ran;
                for (const auto& found : scans) {
                    if (found.ran) {
                        ran.push_back(found.kind);
                    }
                }
                if (ran.size() == scans.size() && scans.size() > 1) {
                    ctx.out << "No cameras answered." << std::endl;
                } else if (ran.size() == 1) {
                    ctx.out << "No " << ran.front() << " cameras answered." << std::endl;
                }
            } else {
                const int IndexColWidth = 3;
                const int TypeColWidth = 8;
                const int CameraColWidth = 21;
                const int ReplyColWidth = 24;
                ctx.out << std::endl << std::left
                        << std::setw(IndexColWidth) << "#" << " | "
                        << std::setw(TypeColWidth) << "Type" << " | "
                        << std::setw(CameraColWidth) << "Camera" << " | "
                        << std::setw(ReplyColWidth) << "Reply" << " | " << "Notes" << std::endl
                        << std::string(IndexColWidth, '-') << "-+-"
                        << std::string(TypeColWidth, '-') << "-+-"
                        << std::string(CameraColWidth, '-') << "-+-"
                        << std::string(ReplyColWidth, '-') << "-+-" << std::string(20, '-') << std::endl;
                for (size_t i = 0; i < rows.size(); ++i) {
                    const auto& row = *rows[i];
                    ctx.out << std::left
                            << std::setw(IndexColWidth) << (i + 1) << " | "
                            << std::setw(TypeColWidth) << (row.serial ? "Serial" : "VISCA-IP") << " | "
                            << std::setw(CameraColWidth) << row.camera << " | "
                            << std::setw(ReplyColWidth) << row.reply << " | "
                            << (row.notes.empty() ? "-" : row.notes) << std::endl;
                }

                const auto serialRows = std::count_if(rows.begin(), rows.end(), [](const Row* r) { return r->serial; });
                const auto ipRows = static_cast<long>(rows.size()) - serialRows;
                const char* plural = rows.size() == 1 ? " camera" : " cameras";
                ctx.out << std::endl << "Found " << rows.size();
                if (ipRows > 0 && serialRows > 0) {
                    ctx.out << plural << ": " << ipRows << " VISCA-IP, " << serialRows << " serial." << std::endl;
                } else {
                    ctx.out << (ipRows > 0 ? " VISCA-IP" : " serial") << plural << "." << std::endl;
                }
            }

            for (const auto& found : scans) {
                for (const auto& remark : found.remarks) {
                    ctx.out << remark << std::endl;
                }
            }

            const bool newIp = std::any_of(rows.begin(), rows.end(), [](const Row* r) { return !r->serial && !r->added; });
            const bool newSerial = std::any_of(rows.begin(), rows.end(), [](const Row* r) { return r->serial && !r->added; });
            if (newIp || newSerial) {
                ctx.out << "Add with: camera add scanned <n...|*> [profile] (default ";
                if (newIp && newSerial) {
                    ctx.out << "tenveo-30x for VISCA-IP, " << visca::kSerialDefaultProfile << " for serial";
                } else {
                    ctx.out << (newIp ? "tenveo-30x" : visca::kSerialDefaultProfile);
                }
                ctx.out << ")" << std::endl;
            }
        }

        void scanIp(data::Context& ctx, const std::vector<std::string>& args) {
            std::string error;
            const auto plan = planIp(args, error);
            if (!plan) {
                ctx.err << error << std::endl;
                return;
            }
            ctx.out << "Scanning " << plan->describe() << "..." << std::endl;
            const auto outcome = runIp(*plan);
            if (!outcome.error.empty()) {
                ctx.err << "Scan failed: " << outcome.error << std::endl;
                return;
            }
            report(ctx, {foundIp(ctx, outcome)});
        }

        void scanSerial(data::Context& ctx, const std::vector<std::string>& args) {
            std::string error;
            const auto plan = planSerial(ctx, args, error);
            if (!plan) {
                ctx.err << error << std::endl;
                return;
            }
            if (!plan->toScan.empty()) {
                ctx.out << "Scanning " << plan->describe() << "..." << std::endl;
            }
            report(ctx, {foundSerial(*plan, visca::scanViscaSerial(plan->toScan))});
        }

        // Both scans at once, with the defaults. A scan that can't run or
        // fails is reported under the table; the other still shows.
        void scanAll(data::Context& ctx) {
            std::string ipError;
            const auto ipPlan = planIp({}, ipError);
            // Without port names there's nothing to reject.
            std::string unused;
            const auto serialPlan = *planSerial(ctx, {}, unused);

            if (ipPlan) {
                ctx.out << "Scanning VISCA-IP: " << ipPlan->describe() << std::endl;
            }
            if (!serialPlan.toScan.empty()) {
                ctx.out << "Scanning serial: " << serialPlan.describe() << std::endl;
            }

            std::future<IpOutcome> ip;
            if (ipPlan) {
                ip = std::async(std::launch::async, runIp, *ipPlan);
            }
            const auto serialResults = visca::scanViscaSerial(serialPlan.toScan);

            Found ipFound;
            ipFound.kind = "VISCA-IP";
            if (ipPlan) {
                ipFound = foundIp(ctx, ip.get());
            } else {
                ipFound.remarks.push_back("VISCA-IP not scanned: " + ipError);
            }
            report(ctx, {ipFound, foundSerial(serialPlan, serialResults)});
        }
    }

    void ScanCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        const std::vector<std::string> rest(tokens.empty() ? tokens.end() : tokens.begin() + 1, tokens.end());
        if (!tokens.empty() && tokens[0] == "visca-ip") {
            scanIp(ctx, rest);
        } else if (!tokens.empty() && tokens[0] == "serial") {
            scanSerial(ctx, rest);
        } else if (tokens.size() == 1 && tokens[0] == "*") {
            scanAll(ctx);
        } else {
            ctx.err << "Usage: camera scan visca-ip [subnet] [port]" << std::endl
                    << "       camera scan serial [COMn...]" << std::endl
                    << "       camera scan *" << std::endl;
        }
    }
}
