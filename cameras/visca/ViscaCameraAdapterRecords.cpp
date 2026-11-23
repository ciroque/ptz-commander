#include "ViscaCameraAdapter.h"
#include "ViscaSerialEndpoint.h"

#include <algorithm>

namespace cameras::visca {

    namespace {

        std::string ipSerial(const ViscaIpEndpoint& endpoint) {
            const int port = endpoint.port > 0 ? endpoint.port : 52381;
            return "VISCA-IP:" + endpoint.host + ":" + std::to_string(port);
        }

        ViscaIpEndpoint ipWithDefaults(ViscaIpEndpoint endpoint) {
            if (endpoint.port <= 0) {
                endpoint.port = 52381;
            }
            if (endpoint.profile.empty()) {
                endpoint.profile = "tenveo-30x";
            }
            return endpoint;
        }

        ViscaSerialEndpoint serialWithDefaults(ViscaSerialEndpoint endpoint) {
            if (endpoint.profile.empty()) {
                endpoint.profile = kSerialDefaultProfile;
            }
            if (endpoint.baud <= 0) {
                endpoint.baud = kSerialDefaultBaud;
            }
            return endpoint;
        }

    } // namespace

    bool ViscaCameraAdapter::keepIp(const ViscaIpEndpoint& requested) {
        if (requested.host.empty()) {
            return false;
        }
        const auto endpoint = ipWithDefaults(requested);
        const std::string id = ipSerial(endpoint);
        if (!manager_.findBySerial(id)) {
            return false;
        }
        const auto stored = std::find_if(ipEndpoints_.begin(), ipEndpoints_.end(),
                                         [&id](const ViscaIpEndpoint& ep) { return ipSerial(ep) == id; });
        if (stored != ipEndpoints_.end()) {
            *stored = endpoint;
            return true;
        }
        ipEndpoints_.push_back(endpoint);
        return true;
    }

    bool ViscaCameraAdapter::keepSerial(const ViscaSerialEndpoint& requested) {
        if (requested.port.empty()) {
            return false;
        }
        const auto endpoint = serialWithDefaults(requested);
        const std::string id = serialCameraId(endpoint);
        if (!manager_.findBySerial(id)) {
            return false;
        }
        const auto stored = std::find_if(serialEndpoints_.begin(), serialEndpoints_.end(),
                                         [&id](const ViscaSerialEndpoint& ep) { return serialCameraId(ep) == id; });
        if (stored != serialEndpoints_.end()) {
            *stored = endpoint;
            return true;
        }
        serialEndpoints_.push_back(endpoint);
        return true;
    }

    bool ViscaCameraAdapter::discard(const std::string& serial) {
        const auto ip = std::find_if(ipEndpoints_.begin(), ipEndpoints_.end(),
                                     [&serial](const ViscaIpEndpoint& ep) { return ipSerial(ep) == serial; });
        const auto onSerial
            = std::find_if(serialEndpoints_.begin(), serialEndpoints_.end(),
                           [&serial](const ViscaSerialEndpoint& ep) { return serialCameraId(ep) == serial; });
        if (ip == ipEndpoints_.end() && onSerial == serialEndpoints_.end()) {
            return false;
        }
        if (ip != ipEndpoints_.end()) {
            ipEndpoints_.erase(ip);
        }
        if (onSerial != serialEndpoints_.end()) {
            serialEndpoints_.erase(onSerial);
        }
        manager_.removeCamera(serial);
        return true;
    }

} // namespace cameras::visca
