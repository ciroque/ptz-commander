#ifndef CAMERAS_VISCA_VISCA_IP_SCANNER_H
#define CAMERAS_VISCA_VISCA_IP_SCANNER_H

#include "ViscaCommands.h"
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cameras::visca {

// An IPv4 network: base address and prefix length, host byte order.
struct Ipv4Network {
    uint32_t base = 0;
    int prefix = 32;

    // Addresses to probe: all but the network and broadcast addresses,
    // except for /31 and /32, which have none. Empty for a prefix outside
    // kMinScanPrefix..32.
    std::vector<uint32_t> hosts() const;
    std::string toString() const;
};

// Explicit ranges may be as large as /16 (65,534 hosts).
constexpr int kMinScanPrefix = 16;
// Interface networks wider than this are narrowed to the /24 around the
// interface address, so a large office or VPN network isn't swept by default.
constexpr int kLocalScanPrefix = 24;

std::string ipv4ToString(uint32_t address);

// "a.b.c.d/n", or "a.b.c.d" for one host. Host bits are cleared.
std::optional<Ipv4Network> parseIpv4Network(const std::string& text, std::string& error);

// The networks of every IPv4 interface that is up, excluding loopback and
// link-local, each narrowed to at most kLocalScanPrefix.
std::vector<Ipv4Network> localIpv4Networks();

struct ViscaIpScanHit {
    std::string host;
    int port = 0;
    std::vector<uint8_t> reply;              // the VISCA reply, header stripped
    std::optional<PanTiltRaw> position;      // set when the reply was a position
};

// Sends a VISCA-over-IP pan/tilt position inquiry to every host in the
// networks on the given port and collects replies. Any VISCA reply counts,
// error replies included: it proves a VISCA device is listening. The
// inquiry does not move anything. Hits are sorted by address.
std::vector<ViscaIpScanHit> scanViscaIp(const std::vector<Ipv4Network>& networks, int port,
                                        std::string& error);

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_IP_SCANNER_H
