//
// Created by steve on 2/24/2025.
//

#ifndef CONTEXT_H
#define CONTEXT_H
#include <windows.h>

namespace data {

class Context {
public:
    HMODULE dllHandle = nullptr;
    int selectedDevice = -1; // -1 means no device selected

    Context() {
        // Load the DLL at startup
        dllHandle = nullptr; // LoadLibrary(TEXT("DeviceLib.dll"));
        // if (!dllHandle) {
        //     std::cerr << "Failed to load DeviceLib.dll\n";
        // }
    }

    ~Context() {
        if (dllHandle) {
            FreeLibrary(dllHandle);
        }
    }

};

} // data

#endif //CONTEXT_H
