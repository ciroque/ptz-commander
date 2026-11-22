#ifndef CAMERAS_ONVIF_ONVIF_CLIENT_H
#define CAMERAS_ONVIF_ONVIF_CLIENT_H

#include <chrono>
#include <string>
#include <utility>

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

    // Probe and move both surface a rejected login with this text.
    inline constexpr const char* kAuthRejected = "the camera rejected the username or password";

    // What failed, separate from the sentence shown to the user. AuthRejected
    // stays AuthRejected if that sentence is reworded.
    enum class OnvifError { None, AuthRejected, NoAnswer, Http, Fault, NoPtzProfile };

    struct OnvifCallResult {
        bool ok = false;
        OnvifError kind = OnvifError::None;
        std::string detail;

        static OnvifCallResult success() {
            OnvifCallResult result;
            result.ok = true;
            return result;
        }

        static OnvifCallResult failure(OnvifError kind, std::string detail) {
            OnvifCallResult result;
            result.kind = kind;
            result.detail = std::move(detail);
            return result;
        }
    };

    // Talks to one camera: WS-Security digest against the camera clock, then
    // AbsoluteMove / GetStatus on the PTZ service advertised by GetCapabilities.
    class OnvifClient {
    public:
        OnvifClient(std::string host, int port, std::string servicePath, std::string user, std::string password);

        OnvifCallResult prepare(OnvifDeviceInfo& info);
        OnvifCallResult absoluteMove(double panX, double tiltY, double zoomX, double panTiltSpeed, double zoomSpeed);
        OnvifCallResult getStatus(OnvifPosition& position);

    private:
        struct SoapResult {
            bool ok = false;
            int status = 0;
            std::string body;
            std::string error;
            OnvifError kind = OnvifError::None;
        };

        OnvifCallResult syncClock();
        OnvifCallResult failureOf(const SoapResult& response, const char* fallback, OnvifError fallbackKind);
        SoapResult post(const std::string& host, int port, const std::string& path, const std::string& inner,
                        bool authenticate);
        SoapResult callAuthenticated(const std::string& host, int port, const std::string& path,
                                     const std::string& inner);
        OnvifCallResult loadServices();
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
