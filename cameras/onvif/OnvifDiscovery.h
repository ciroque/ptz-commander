#ifndef CAMERAS_ONVIF_ONVIF_DISCOVERY_H
#define CAMERAS_ONVIF_ONVIF_DISCOVERY_H

#include "OnvifEndpoint.h"

namespace cameras::onvif {

// WS-Discovery probe to 239.255.255.250:3702. Replies are unicast back to the
// socket that sent the probe. Does not add cameras and does not use credentials.
class OnvifDiscovery {
public:
    static OnvifScanResult probe(int timeoutMs = 2500);
};

} // namespace cameras::onvif

#endif
