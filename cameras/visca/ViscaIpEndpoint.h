#ifndef CAMERAS_VISCA_VISCA_IP_ENDPOINT_H
#define CAMERAS_VISCA_VISCA_IP_ENDPOINT_H

#include <string>

namespace cameras::visca {

struct ViscaIpEndpoint {
    std::string host;
    int port = 52381;
    std::string profile = "tenveo-30x";
};

} // namespace cameras::visca

#endif
