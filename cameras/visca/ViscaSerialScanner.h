#ifndef CAMERAS_VISCA_VISCA_SERIAL_SCANNER_H
#define CAMERAS_VISCA_VISCA_SERIAL_SCANNER_H

#include "ViscaCommands.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace cameras::visca {

struct SerialPortInfo {
    std::string port;          // "COM3"
    std::string description;   // the device's name without "(COM3)"; empty if unknown
};

// The COM ports Windows reports right now, in numeric order. A USB adapter
// is only listed while it's plugged in.
std::vector<SerialPortInfo> listSerialPorts();

// The rates a scan tries, in order: the Lumens VC-A51P's two.
inline const std::vector<int>& serialScanBauds() {
    static const std::vector<int> bauds{9600, 38400};
    return bauds;
}

struct ViscaSerialScanResult {
    enum class Status {
        Answered,
        Silent,    // opened, but nothing answered at any rate or address
        Busy,      // another program has the port open
        Failed     // couldn't be opened for another reason (see openError)
    };
    SerialPortInfo port;
    Status status = Status::Silent;
    int baud = 0;                          // when Answered
    int address = 0;                       // when Answered
    std::vector<uint8_t> reply;            // when Answered
    std::optional<PanTiltRaw> position;    // when the reply was a position
    unsigned long openError = 0;           // Windows error code, when Busy or Failed
};

// Sends a pan/tilt position inquiry to camera addresses 1-7 at each rate
// in serialScanBauds(), stopping at the first that answers; any reply
// from the address asked counts. The inquiry doesn't move anything. Ports
// are probed in parallel; results are in the order given. A silent port
// takes about 2.5 s.
std::vector<ViscaSerialScanResult> scanViscaSerial(const std::vector<SerialPortInfo>& ports);

} // namespace cameras::visca

#endif // CAMERAS_VISCA_VISCA_SERIAL_SCANNER_H
