#ifndef CAMERAS_ONVIF_ONVIF_CLIENT_H
#define CAMERAS_ONVIF_ONVIF_CLIENT_H

#include <chrono>
#include <string>

namespace cameras::onvif {

struct OnvifDeviceInfo {
    std::string manufacturer;
    std::string model;
    std::string firmware;
    std::string serialNumber;
    std::string hardwareId;
};

struct OnvifPosition {
    double panX = 0;
    double tiltY = 0;
    double zoomX = 0;
};

// Talks to one camera: WS-Security digest against the camera clock, then
// AbsoluteMove / GetStatus on the PTZ service advertised by GetCapabilities.
class OnvifClient {
public:
    OnvifClient(std::string host, int port, std::string servicePath,
                std::string user, std::string password);

    bool prepare(OnvifDeviceInfo& info, std::string& error);
    bool absoluteMove(double panX, double tiltY, double zoomX,
                      double panTiltSpeed, double zoomSpeed, std::string& error);
    bool getStatus(OnvifPosition& position, std::string& error);

private:
    struct SoapResult {
        bool ok = false;
        int status = 0;
        std::string body;
        std::string error;
    };

    bool syncClock(std::string& error);
    SoapResult post(const std::string& host, int port, const std::string& path,
                    const std::string& inner, bool authenticate);
    SoapResult callAuthenticated(const std::string& host, int port, const std::string& path,
                                 const std::string& inner);
    bool loadServices(std::string& error);
    static bool isFault(const std::string& body);
    static bool isAuthFault(const std::string& body);
    static std::string faultText(const std::string& body);

    std::string host_;
    int port_ = 80;
    std::string servicePath_;
    std::string user_;
    std::string password_;

    std::string mediaHost_;
    int mediaPort_ = 80;
    std::string mediaPath_;
    std::string ptzHost_;
    int ptzPort_ = 80;
    std::string ptzPath_;
    std::string profileToken_;
    bool ready_ = false;

    std::chrono::system_clock::duration clockOffset_{0};
    bool clockSynced_ = false;
};

} // namespace cameras::onvif

#endif
