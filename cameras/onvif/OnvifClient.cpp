#include "OnvifClient.h"

#include "OnvifParse.h"
#include "../../core/Logger.h"

#include <chrono>
#include <ctime>
#include <sstream>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>

namespace cameras::onvif {

namespace {

std::wstring widen(const std::string& text) {
    if (text.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    if (size <= 0) {
        return {};
    }
    std::wstring out(static_cast<size_t>(size), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), out.data(), size);
    return out;
}

std::string base64(const unsigned char* data, size_t len) {
    static const char* alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve((len + 2) / 3 * 4);
    for (size_t i = 0; i < len; i += 3) {
        unsigned int value = static_cast<unsigned int>(data[i]) << 16;
        if (i + 1 < len) {
            value |= static_cast<unsigned int>(data[i + 1]) << 8;
        }
        if (i + 2 < len) {
            value |= static_cast<unsigned int>(data[i + 2]);
        }
        out.push_back(alphabet[(value >> 18) & 63]);
        out.push_back(alphabet[(value >> 12) & 63]);
        out.push_back(i + 1 < len ? alphabet[(value >> 6) & 63] : '=');
        out.push_back(i + 2 < len ? alphabet[value & 63] : '=');
    }
    return out;
}

bool sha1(const unsigned char* data, size_t len, unsigned char out[20]) {
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA1_ALGORITHM, nullptr, 0) != 0) {
        return false;
    }
    const bool created = BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) == 0;
    bool ok = created
        && BCryptHashData(hash, const_cast<unsigned char*>(data), static_cast<ULONG>(len), 0) == 0
        && BCryptFinishHash(hash, out, 20, 0) == 0;
    if (hash) {
        BCryptDestroyHash(hash);
    }
    BCryptCloseAlgorithmProvider(algorithm, 0);
    return ok;
}

std::string createdTimestamp(std::chrono::system_clock::duration offset) {
    const auto when = std::chrono::system_clock::now() + offset;
    const std::time_t seconds = std::chrono::system_clock::to_time_t(when);
    std::tm utc{};
    gmtime_s(&utc, &seconds);
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                  utc.tm_year + 1900, utc.tm_mon + 1, utc.tm_mday,
                  utc.tm_hour, utc.tm_min, utc.tm_sec);
    return buf;
}

std::string securityHeader(const std::string& user, const std::string& password,
                           std::chrono::system_clock::duration offset) {
    unsigned char nonce[16];
    if (BCryptGenRandom(nullptr, nonce, sizeof(nonce), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0) {
        return {};
    }
    const std::string created = createdTimestamp(offset);
    std::vector<unsigned char> material;
    material.insert(material.end(), nonce, nonce + sizeof(nonce));
    material.insert(material.end(), created.begin(), created.end());
    material.insert(material.end(), password.begin(), password.end());
    unsigned char digest[20];
    if (!sha1(material.data(), material.size(), digest)) {
        return {};
    }

    std::ostringstream header;
    header << "<wsse:Security s:mustUnderstand=\"1\">"
           << "<wsse:UsernameToken>"
           << "<wsse:Username>" << xmlEscape(user) << "</wsse:Username>"
           << "<wsse:Password Type=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-username-token-profile-1.0#PasswordDigest\">"
           << base64(digest, sizeof(digest)) << "</wsse:Password>"
           << "<wsse:Nonce EncodingType=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-soap-message-security-1.0#Base64Binary\">"
           << base64(nonce, sizeof(nonce)) << "</wsse:Nonce>"
           << "<wsu:Created>" << created << "</wsu:Created>"
           << "</wsse:UsernameToken></wsse:Security>";
    return header.str();
}

std::string envelope(const std::string& header, const std::string& body) {
    std::ostringstream xml;
    xml << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
        << "<s:Envelope xmlns:s=\"http://www.w3.org/2003/05/soap-envelope\" "
        << "xmlns:wsse=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-secext-1.0.xsd\" "
        << "xmlns:wsu=\"http://docs.oasis-open.org/wss/2004/01/oasis-200401-wss-wssecurity-utility-1.0.xsd\">"
        << "<s:Header>" << header << "</s:Header><s:Body>" << body << "</s:Body></s:Envelope>";
    return xml.str();
}

struct WinHttpHandle {
    HINTERNET handle = nullptr;
    explicit WinHttpHandle(HINTERNET value) : handle(value) {}
    ~WinHttpHandle() {
        if (handle) {
            WinHttpCloseHandle(handle);
        }
    }
    WinHttpHandle(const WinHttpHandle&) = delete;
    WinHttpHandle& operator=(const WinHttpHandle&) = delete;
};

bool parseServiceUrl(const std::string& xaddr, const std::string& fallbackHost,
                     std::string& host, int& port, std::string& path, std::string& error) {
    const auto parsed = parseHttpUrl(trimCopy(xaddr));
    if (!parsed) {
        error = "camera returned an unrecognized service address";
        return false;
    }
    if (parsed->https) {
        error = "HTTPS ONVIF is not supported";
        return false;
    }
    host = fallbackHost.empty() ? parsed->host : fallbackHost;
    port = parsed->port;
    path = parsed->path.empty() ? "/" : parsed->path;
    return true;
}

int fieldNumber(const std::string& xml, const std::string& name) {
    const std::string text = xmlElementText(xml, name);
    if (text.empty()) {
        return -1;
    }
    int value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return -1;
        }
        value = value * 10 + (c - '0');
    }
    return value;
}

} // namespace

OnvifClient::OnvifClient(std::string host, int port, std::string servicePath,
                         std::string user, std::string password)
    : host_(std::move(host))
    , port_(port > 0 ? port : 80)
    , servicePath_(std::move(servicePath))
    , user_(std::move(user))
    , password_(std::move(password)) {
    if (servicePath_.empty() || servicePath_.front() != '/') {
        servicePath_.insert(servicePath_.begin(), '/');
    }
}

bool OnvifClient::isFault(const std::string& body) {
    return body.find(":Fault") != std::string::npos || body.find("<Fault") != std::string::npos;
}

bool OnvifClient::isAuthFault(const std::string& body) {
    return body.find("NotAuthorized") != std::string::npos
        || body.find("FailedAuthentication") != std::string::npos
        || body.find("not Authorized") != std::string::npos;
}

std::string OnvifClient::faultText(const std::string& body) {
    const std::string text = xmlElementText(body, "Text");
    if (!text.empty()) {
        return text;
    }
    const std::string subcode = xmlElementText(body, "Value");
    if (!subcode.empty()) {
        return subcode;
    }
    return "the camera rejected the request";
}

OnvifClient::SoapResult OnvifClient::post(const std::string& host, int port, const std::string& path,
                                          const std::string& inner, bool authenticate) {
    SoapResult result;
    std::string header;
    if (authenticate) {
        header = securityHeader(user_, password_, clockOffset_);
        if (header.empty()) {
            result.error = "could not build the ONVIF login token";
            return result;
        }
    }

    const std::string body = envelope(header, inner);
    const std::wstring hostWide = widen(host);
    const std::wstring pathWide = widen(path.empty() ? "/" : path);
    if (hostWide.empty() || pathWide.empty()) {
        result.error = "the ONVIF address is not valid";
        return result;
    }

    WinHttpHandle session(WinHttpOpen(L"PTZCommander", WINHTTP_ACCESS_TYPE_NO_PROXY,
                                      WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0));
    if (!session.handle) {
        result.error = "could not open an HTTP session";
        return result;
    }
    WinHttpSetTimeouts(session.handle, 3000, 3000, 5000, 8000);

    WinHttpHandle connection(WinHttpConnect(session.handle, hostWide.c_str(),
                                            static_cast<INTERNET_PORT>(port), 0));
    if (!connection.handle) {
        result.error = "could not connect to " + host;
        return result;
    }

    WinHttpHandle request(WinHttpOpenRequest(connection.handle, L"POST", pathWide.c_str(),
                                             nullptr, WINHTTP_NO_REFERER,
                                             WINHTTP_DEFAULT_ACCEPT_TYPES, 0));
    if (!request.handle) {
        result.error = "could not create the ONVIF request";
        return result;
    }

    const wchar_t* headers = L"Content-Type: application/soap+xml; charset=utf-8\r\n";
    if (!WinHttpSendRequest(request.handle, headers, static_cast<DWORD>(-1),
                            const_cast<char*>(body.data()), static_cast<DWORD>(body.size()),
                            static_cast<DWORD>(body.size()), 0)
        || !WinHttpReceiveResponse(request.handle, nullptr)) {
        result.error = "the ONVIF request to " + host + " failed";
        return result;
    }

    DWORD status = 0;
    DWORD statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(request.handle, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize,
                             WINHTTP_NO_HEADER_INDEX)) {
        result.error = "could not read the ONVIF response";
        return result;
    }
    result.status = static_cast<int>(status);

    std::string payload;
    for (;;) {
        DWORD available = 0;
        if (!WinHttpQueryDataAvailable(request.handle, &available)) {
            result.error = "could not read the ONVIF response";
            return result;
        }
        if (available == 0) {
            break;
        }
        if (payload.size() + available > 1024 * 1024) {
            result.error = "the ONVIF response was too large";
            return result;
        }
        const size_t offset = payload.size();
        payload.resize(offset + available);
        DWORD read = 0;
        if (!WinHttpReadData(request.handle, payload.data() + offset, available, &read)) {
            result.error = "could not read the ONVIF response";
            return result;
        }
        payload.resize(offset + read);
        if (read == 0) {
            break;
        }
    }

    result.body = std::move(payload);
    result.ok = true;
    if (result.status == 401 || isAuthFault(result.body)) {
        result.error = "the camera rejected the username or password";
    } else if (result.status != 200) {
        result.error = "the camera returned HTTP " + std::to_string(result.status);
    } else if (isFault(result.body)) {
        result.error = faultText(result.body);
    }
    return result;
}

bool OnvifClient::syncClock(std::string& error) {
    const auto response = post(host_, port_, servicePath_,
                               "<tds:GetSystemDateAndTime xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\"/>",
                               false);
    if (!response.ok || response.status != 200 || isFault(response.body)) {
        error = response.error.empty() ? "could not read the camera clock" : response.error;
        return false;
    }

    const size_t utc = findXmlTag(response.body, "UTCDateTime");
    const std::string block = utc == std::string::npos ? response.body : response.body.substr(utc);
    const int year = fieldNumber(block, "Year");
    const int month = fieldNumber(block, "Month");
    const int day = fieldNumber(block, "Day");
    const int hour = fieldNumber(block, "Hour");
    const int minute = fieldNumber(block, "Minute");
    const int second = fieldNumber(block, "Second");
    if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31
        || hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60) {
        error = "the camera clock was not readable";
        return false;
    }

    std::tm utcTm{};
    utcTm.tm_year = year - 1900;
    utcTm.tm_mon = month - 1;
    utcTm.tm_mday = day;
    utcTm.tm_hour = hour;
    utcTm.tm_min = minute;
    utcTm.tm_sec = second;
    const std::time_t cameraTime = _mkgmtime(&utcTm);
    if (cameraTime == static_cast<std::time_t>(-1)) {
        error = "the camera clock was not readable";
        return false;
    }
    const auto cameraPoint = std::chrono::system_clock::from_time_t(cameraTime);
    clockOffset_ = cameraPoint - std::chrono::system_clock::now();
    clockSynced_ = true;
    return true;
}

OnvifClient::SoapResult OnvifClient::callAuthenticated(const std::string& host, int port,
                                                       const std::string& path, const std::string& inner) {
    SoapResult response;
    for (int attempt = 0; attempt < 2; ++attempt) {
        std::string clockError;
        if (!clockSynced_ && !syncClock(clockError)) {
            response.error = clockError;
            return response;
        }
        response = post(host, port, path, inner, true);
        if (response.ok && response.status == 200 && !isFault(response.body)) {
            return response;
        }
        if (attempt == 0 && (response.status == 401 || isAuthFault(response.body))) {
            clockSynced_ = false;
            continue;
        }
        return response;
    }
    return response;
}

bool OnvifClient::loadServices(std::string& error) {
    const auto caps = callAuthenticated(
        host_, port_, servicePath_,
        "<tds:GetCapabilities xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\"><tds:Category>All</tds:Category></tds:GetCapabilities>");
    if (!caps.ok || caps.status != 200 || isFault(caps.body)) {
        error = caps.error.empty() ? "could not read the camera services" : caps.error;
        return false;
    }

    const size_t mediaAt = findXmlTag(caps.body, "Media");
    const size_t ptzAt = findXmlTag(caps.body, "PTZ");
    if (mediaAt == std::string::npos || ptzAt == std::string::npos) {
        error = "the camera has no ONVIF PTZ service";
        return false;
    }
    const std::string mediaXaddr = xmlElementText(caps.body, "XAddr", mediaAt);
    const std::string ptzXaddr = xmlElementText(caps.body, "XAddr", ptzAt);
    if (!parseServiceUrl(mediaXaddr, host_, mediaHost_, mediaPort_, mediaPath_, error)
        || !parseServiceUrl(ptzXaddr, host_, ptzHost_, ptzPort_, ptzPath_, error)) {
        return false;
    }

    const auto profiles = callAuthenticated(
        mediaHost_, mediaPort_, mediaPath_,
        "<trt:GetProfiles xmlns:trt=\"http://www.onvif.org/ver10/media/wsdl\"/>");
    if (!profiles.ok || profiles.status != 200 || isFault(profiles.body)) {
        error = profiles.error.empty() ? "could not read the camera profiles" : profiles.error;
        return false;
    }

    const size_t profileAt = profiles.body.find("Profiles token=\"");
    if (profileAt == std::string::npos) {
        error = "the camera has no PTZ profile";
        return false;
    }
    profileToken_ = xmlAttr(profiles.body.substr(profileAt, 80), "token");
    if (profileToken_.empty()) {
        const size_t quote = profiles.body.find('"', profileAt + 16);
        if (quote != std::string::npos) {
            profileToken_ = profiles.body.substr(profileAt + 16, quote - (profileAt + 16));
        }
    }
    if (profileToken_.empty()) {
        error = "the camera has no PTZ profile";
        return false;
    }
    return true;
}

bool OnvifClient::prepare(OnvifDeviceInfo& info, std::string& error) {
    const auto device = callAuthenticated(
        host_, port_, servicePath_,
        "<tds:GetDeviceInformation xmlns:tds=\"http://www.onvif.org/ver10/device/wsdl\"/>");
    if (!device.ok || device.status != 200 || isFault(device.body)) {
        error = device.error.empty() ? "the camera did not answer" : device.error;
        return false;
    }
    info.manufacturer = xmlElementText(device.body, "Manufacturer");
    info.model = xmlElementText(device.body, "Model");
    info.firmware = xmlElementText(device.body, "FirmwareVersion");
    info.serialNumber = xmlElementText(device.body, "SerialNumber");
    info.hardwareId = xmlElementText(device.body, "HardwareId");

    if (!loadServices(error)) {
        return false;
    }
    ready_ = true;
    core::Logger::info("ONVIF ready " + host_ + ":" + std::to_string(port_)
                       + " " + info.manufacturer + " " + info.model);
    return true;
}

bool OnvifClient::absoluteMove(double panX, double tiltY, double zoomX,
                               double panTiltSpeed, double zoomSpeed, std::string& error) {
    if (!ready_) {
        error = "the camera is not ready";
        return false;
    }
    const std::string token = xmlEscape(profileToken_);
    const std::string body =
        "<tptz:AbsoluteMove xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\">"
        "<tptz:ProfileToken>" + token + "</tptz:ProfileToken>"
        "<tptz:Position>"
        "<tt:PanTilt xmlns:tt=\"http://www.onvif.org/ver10/schema\" x=\"" + formatFixed(panX)
        + "\" y=\"" + formatFixed(tiltY) + "\"/>"
        "<tt:Zoom xmlns:tt=\"http://www.onvif.org/ver10/schema\" x=\"" + formatFixed(zoomX) + "\"/>"
        "</tptz:Position>"
        "<tptz:Speed>"
        "<tt:PanTilt xmlns:tt=\"http://www.onvif.org/ver10/schema\" x=\"" + formatFixed(panTiltSpeed)
        + "\" y=\"" + formatFixed(panTiltSpeed) + "\"/>"
        "<tt:Zoom xmlns:tt=\"http://www.onvif.org/ver10/schema\" x=\"" + formatFixed(zoomSpeed) + "\"/>"
        "</tptz:Speed></tptz:AbsoluteMove>";

    const auto response = callAuthenticated(ptzHost_, ptzPort_, ptzPath_, body);
    if (!response.ok || response.status != 200 || isFault(response.body)) {
        error = response.error.empty() ? "the move was rejected" : response.error;
        return false;
    }
    return true;
}

bool OnvifClient::getStatus(OnvifPosition& position, std::string& error) {
    if (!ready_) {
        error = "the camera is not ready";
        return false;
    }
    const std::string body =
        "<tptz:GetStatus xmlns:tptz=\"http://www.onvif.org/ver20/ptz/wsdl\">"
        "<tptz:ProfileToken>" + xmlEscape(profileToken_) + "</tptz:ProfileToken>"
        "</tptz:GetStatus>";
    const auto response = callAuthenticated(ptzHost_, ptzPort_, ptzPath_, body);
    if (!response.ok || response.status != 200 || isFault(response.body)) {
        error = response.error.empty() ? "could not read the camera position" : response.error;
        return false;
    }

    const std::string panTilt = xmlStartTag(response.body, "PanTilt");
    const std::string zoom = xmlStartTag(response.body, "Zoom");
    const std::string xText = xmlAttr(panTilt, "x");
    const std::string yText = xmlAttr(panTilt, "y");
    const std::string zText = xmlAttr(zoom, "x");
    if (xText.empty() || yText.empty() || zText.empty()) {
        error = "the camera position was not readable";
        return false;
    }
    try {
        position.panX = std::stod(xText);
        position.tiltY = std::stod(yText);
        position.zoomX = std::stod(zText);
    } catch (const std::exception&) {
        error = "the camera position was not readable";
        return false;
    }
    return true;
}

} // namespace cameras::onvif
