#include "ViscaDiscovery.h"

#ifdef _WIN32

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <algorithm>

#include "../../core/Logger.h"
#include "SerialTransport.h"           // for probe attempt
#include "WindowsSerialTransport.h"
#include "ViscaCommands.h"

namespace cameras::visca {

namespace {

// Enumerate COM ports using the registry (convenient but not always populated).
std::vector<std::string> enumerateComPortsFromRegistry() {
    std::vector<std::string> ports;
    HKEY hKey = nullptr;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                      "HARDWARE\\DEVICEMAP\\SERIALCOMM",
                      0,
                      KEY_READ,
                      &hKey) != ERROR_SUCCESS) {
        return ports;
    }

    char valueName[256];
    char portName[256];
    DWORD valueNameSize, portNameSize, type;

    for (DWORD i = 0; ; ++i) {
        valueNameSize = sizeof(valueName);
        portNameSize  = sizeof(portName);

        LONG res = RegEnumValueA(hKey, i, valueName, &valueNameSize,
                                 nullptr, &type,
                                 reinterpret_cast<LPBYTE>(portName), &portNameSize);

        if (res == ERROR_NO_MORE_ITEMS) break;
        if (res == ERROR_SUCCESS && type == REG_SZ) {
            std::string p(portName);
            if (p.rfind("COM", 0) == 0) {
                ports.push_back(p);
            }
        }
    }

    RegCloseKey(hKey);

    // Sort for stable order (COM3 before COM10 etc.)
    std::sort(ports.begin(), ports.end(), [](const std::string& a, const std::string& b) {
        int na = 0, nb = 0;
        sscanf_s(a.c_str(), "COM%d", &na);
        sscanf_s(b.c_str(), "COM%d", &nb);
        return na < nb;
    });

    return ports;
}

// Robust fallback: probe COM1..COM256 using QueryDosDevice.
// This works even when the SERIALCOMM registry key is empty or incomplete.
std::vector<std::string> enumerateComPortsViaQueryDosDevice() {
    std::vector<std::string> ports;
    for (int i = 1; i <= 256; ++i) {
        char portName[16];
        sprintf_s(portName, sizeof(portName), "COM%d", i);
        char targetPath[256] = {0};
        if (QueryDosDeviceA(portName, targetPath, sizeof(targetPath)) != 0) {
            ports.push_back(portName);
        }
    }
    return ports;
}

// Enumerate ports by walking the SetupAPI Ports class and parsing "(COMn)" from FriendlyName.
// This is often the most reliable for USB serial adapters (Keyspan, FTDI, etc.).
std::vector<std::string> enumerateComPortsViaSetupApiFriendly() {
    std::vector<std::string> ports;

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsA(
        &GUID_DEVCLASS_PORTS,
        nullptr,
        nullptr,
        DIGCF_PRESENT);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return ports;
    }

    SP_DEVINFO_DATA devInfoData{};
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &devInfoData); ++i) {
        char friendly[512] = {0};
        if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &devInfoData,
                                              SPDRP_FRIENDLYNAME,
                                              nullptr,
                                              reinterpret_cast<PBYTE>(friendly),
                                              sizeof(friendly), nullptr)) {
            std::string f(friendly);
            size_t pos = f.find("(COM");
            if (pos != std::string::npos) {
                size_t end = f.find(')', pos);
                if (end != std::string::npos) {
                    std::string port = f.substr(pos + 1, end - pos - 1);
                    if (port.rfind("COM", 0) == 0) {
                        ports.push_back(port);
                    }
                }
            }
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    // Sort numerically and dedup
    std::sort(ports.begin(), ports.end(), [](const std::string& a, const std::string& b) {
        int na = 0, nb = 0;
        sscanf_s(a.c_str(), "COM%d", &na);
        sscanf_s(b.c_str(), "COM%d", &nb);
        return na < nb;
    });
    auto last = std::unique(ports.begin(), ports.end());
    ports.erase(last, ports.end());

    return ports;
}

// Use SetupAPI to enrich port info with VID/PID and friendly name.
// Returns map from port name -> candidate with vid/pid/desc.
struct PortInfo {
    uint16_t vid = 0;
    uint16_t pid = 0;
    std::string description;
};

std::vector<PortInfo> getPortInfoViaSetupApi(const std::string& targetPort) {
    // We collect info for the specific port (or all if targetPort empty).
    std::vector<PortInfo> results;

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsA(
        &GUID_DEVCLASS_PORTS,
        nullptr,
        nullptr,
        DIGCF_PRESENT);

    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return results;
    }

    SP_DEVINFO_DATA devInfoData{};
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &devInfoData); ++i) {
        char friendly[512] = {0};
        char hardwareIds[512] = {0};

        // Friendly name (often contains "(COMx)")
        bool haveFriendly = SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &devInfoData,
                                              SPDRP_FRIENDLYNAME,
                                              nullptr,
                                              reinterpret_cast<PBYTE>(friendly),
                                              sizeof(friendly), nullptr);

        // Hardware IDs contain VID_xxxx&PID_xxxx (optional)
        uint16_t vid = 0, pid = 0;
        if (SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &devInfoData,
                                              SPDRP_HARDWAREID,
                                              nullptr,
                                              reinterpret_cast<PBYTE>(hardwareIds),
                                              sizeof(hardwareIds), nullptr)) {

            std::string hwid(hardwareIds);
            auto lower = hwid;
            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            size_t vpos = lower.find("vid_");
            size_t ppos = lower.find("pid_");
            if (vpos != std::string::npos && ppos != std::string::npos) {
                sscanf_s(hwid.c_str() + vpos + 4, "%4hx", &vid);
                sscanf_s(hwid.c_str() + ppos + 4, "%4hx", &pid);
            }
        }

        // Extract port from friendly name if present (do this independently of VID parse)
        std::string port;
        if (haveFriendly) {
            std::string f(friendly);
            size_t paren = f.find("(COM");
            if (paren != std::string::npos) {
                port = f.substr(paren + 1);
                size_t end = port.find(')');
                if (end != std::string::npos) port = port.substr(0, end);
            }
        }

        if (!port.empty() && (targetPort.empty() || port == targetPort)) {
            PortInfo info;
            info.vid = vid;
            info.pid = pid;
            info.description = haveFriendly ? friendly : ("Port " + port);
            results.push_back(info);
        }
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return results;
}

bool isKeyspan(uint16_t vid, uint16_t pid) {
    return (vid == 0x06CD && pid == 0x0121);
}

} // anonymous namespace

std::vector<ViscaPortCandidate> ViscaDiscovery::discover() {
    std::vector<ViscaPortCandidate> candidates;

    auto comPorts = enumerateComPortsFromRegistry();
    if (comPorts.empty()) {
        core::Logger::warn("No COM ports found via registry (this key is often empty). Trying fallbacks...");
        comPorts = enumerateComPortsViaSetupApiFriendly();
        if (comPorts.empty()) {
            comPorts = enumerateComPortsViaQueryDosDevice();
        }
    }

    if (comPorts.empty()) {
        core::Logger::warn("No COM ports found via any method. (Is the USB serial adapter physically connected right now?)");
        return candidates;
    }

    core::Logger::info("Found " + std::to_string(comPorts.size()) + " COM ports currently visible to the system for VISCA discovery");

    for (const auto& port : comPorts) {
        ViscaPortCandidate cand;
        cand.port = port;
        cand.baud = 9600;
        cand.address = 1;

        // Enrich with SetupAPI info
        auto infos = getPortInfoViaSetupApi(port);
        if (!infos.empty()) {
            const auto& info = infos.front();
            cand.vid = info.vid;
            cand.pid = info.pid;
            cand.description = info.description;
            cand.matchesKeyspan = isKeyspan(info.vid, info.pid);
        } else {
            cand.description = "Serial port " + port;
        }

        candidates.push_back(cand);
    }

    // Sort: Keyspan matches first (by preference), then numerically by COM index
    // (avoids "COM10" < "COM2" lexicographic ordering).
    std::sort(candidates.begin(), candidates.end(),
        [](const ViscaPortCandidate& a, const ViscaPortCandidate& b) {
            if (a.matchesKeyspan != b.matchesKeyspan) return a.matchesKeyspan > b.matchesKeyspan;
            int na = 0, nb = 0;
            sscanf_s(a.port.c_str(), "COM%d", &na);
            sscanf_s(b.port.c_str(), "COM%d", &nb);
            return na < nb;
        });

    for (const auto& c : candidates) {
        std::string tag = c.matchesKeyspan ? " [Keyspan USA-19H preferred]" : "";
        core::Logger::info("VISCA candidate: " + c.port + tag + " (" + c.description + ")");
    }

    return candidates;
}

bool ViscaDiscovery::probePort(const ViscaPortCandidate& candidate) {
    // Lightweight "port present" probe: attempt to open the serial port at the
    // candidate's baud rate. Returns true if the open succeeds.
    // No VISCA command is sent and no response is read (full response/ACK
    // handling is still deferred). Callers combine this with prior VID/PID or
    // friendly-name filtering (see matchesKeyspan) to decide whether a
    // candidate is likely a real VISCA device.
    //
    // Future work: send a safe non-motion inquiry (e.g. power or zoom position
    // inquiry) and check that the reply starts with 0x90.

    WindowsSerialTransport transport;

    bool opened = transport.open(candidate.port, candidate.baud);
    transport.close();

    if (!opened) {
        return false;
    }

    if (candidate.matchesKeyspan) {
        core::Logger::info("VISCA probe: Keyspan USA-19H match + port open succeeded: " + candidate.port);
        return true;
    }

    core::Logger::info("VISCA probe: port open succeeded (non-Keyspan): " + candidate.port);
    return true;
}

} // namespace cameras::visca

#endif // _WIN32
