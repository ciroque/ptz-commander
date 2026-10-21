#include "ViscaSerialScanner.h"

#ifdef _WIN32

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <cstdio>
#include <algorithm>
#include <future>
#include <map>

#include "../../core/Logger.h"
#include "WindowsSerialTransport.h"

namespace cameras::visca {

namespace {

// The first inquiry on a port waits longest; a camera that answers at all
// does so within tens of milliseconds at 9600 baud.
constexpr int kFirstReplyMs = 300;
constexpr int kReplyMs = 150;

int comNumber(const std::string& port) {
    int n = 0;
    sscanf_s(port.c_str(), "COM%d", &n);
    return n;
}

// Ports from the registry (convenient but not always populated).
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
    return ports;
}

// Fallback: probe COM1..COM256 using QueryDosDevice.
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

// Names of the present devices in the Ports class, keyed by port, from
// friendly names such as "USB-SERIAL CH340 (COM7)".
std::map<std::string, std::string> portDescriptions() {
    std::map<std::string, std::string> names;

    HDEVINFO deviceInfoSet = SetupDiGetClassDevsA(&GUID_DEVCLASS_PORTS, nullptr, nullptr, DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return names;
    }

    SP_DEVINFO_DATA devInfoData{};
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &devInfoData); ++i) {
        char friendly[512] = {0};
        if (!SetupDiGetDeviceRegistryPropertyA(deviceInfoSet, &devInfoData, SPDRP_FRIENDLYNAME, nullptr,
                                               reinterpret_cast<PBYTE>(friendly), sizeof(friendly), nullptr)) {
            continue;
        }
        const std::string f(friendly);
        const size_t open = f.find("(COM");
        const size_t close = open == std::string::npos ? std::string::npos : f.find(')', open);
        if (close == std::string::npos) {
            continue;
        }
        std::string description = f.substr(0, open);
        while (!description.empty() && description.back() == ' ') {
            description.pop_back();
        }
        names[f.substr(open + 1, close - open - 1)] = description;
    }

    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return names;
}

ViscaSerialScanResult probePort(const SerialPortInfo& info) {
    using Status = ViscaSerialScanResult::Status;
    ViscaSerialScanResult result;
    result.port = info;

    for (const int baud : serialScanBauds()) {
        WindowsSerialTransport transport(info.port, baud, /*quiet=*/true);
        if (!transport.open()) {
            result.openError = transport.lastError();
            result.status = result.openError == ERROR_ACCESS_DENIED ? Status::Busy : Status::Failed;
            return result;
        }
        for (int address = 1; address <= 7; ++address) {
            transport.purgeInput();
            if (!transport.write(ViscaCommands::panTiltPositionInquiry(static_cast<uint8_t>(address)))) {
                break;
            }
            const auto reply = transport.readPacket(address == 1 ? kFirstReplyMs : kReplyMs);
            if (isViscaReply(reply) && reply[0] == replyHeader(static_cast<uint8_t>(address))) {
                result.status = Status::Answered;
                result.baud = baud;
                result.address = address;
                result.reply = reply;
                result.position = ViscaCommands::parsePanTiltPosition(reply);
                core::Logger::info("VISCA serial scan: camera on " + info.port + " @" + std::to_string(baud) +
                                   " address " + std::to_string(address));
                return result;
            }
        }
    }
    result.status = Status::Silent;
    return result;
}

} // anonymous namespace

std::vector<SerialPortInfo> listSerialPorts() {
    const auto descriptions = portDescriptions();

    auto ports = enumerateComPortsFromRegistry();
    if (ports.empty()) {
        for (const auto& entry : descriptions) {
            ports.push_back(entry.first);
        }
    }
    if (ports.empty()) {
        ports = enumerateComPortsViaQueryDosDevice();
    }

    std::sort(ports.begin(), ports.end(), [](const std::string& a, const std::string& b) {
        return comNumber(a) < comNumber(b);
    });
    ports.erase(std::unique(ports.begin(), ports.end()), ports.end());

    std::vector<SerialPortInfo> infos;
    for (const auto& port : ports) {
        const auto it = descriptions.find(port);
        infos.push_back({port, it == descriptions.end() ? std::string() : it->second});
    }
    return infos;
}

std::vector<ViscaSerialScanResult> scanViscaSerial(const std::vector<SerialPortInfo>& ports) {
    std::vector<std::future<ViscaSerialScanResult>> pending;
    for (const auto& port : ports) {
        pending.push_back(std::async(std::launch::async, probePort, port));
    }
    std::vector<ViscaSerialScanResult> results;
    for (auto& future : pending) {
        results.push_back(future.get());
    }
    return results;
}

} // namespace cameras::visca

#endif // _WIN32
