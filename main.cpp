#include "core/Application.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <csignal>
#endif

int main() {
#ifdef _WIN32
    // Ignore Ctrl+C before any SDK threads start. The CRT default SIGINT
    // handler calls exit(); SetConsoleCtrlHandler(NULL, TRUE) is the
    // documented process-wide ignore for CTRL+C.
    std::signal(SIGINT, SIG_IGN);
    SetConsoleCtrlHandler(nullptr, TRUE);
#endif

    core::Application application;
    application.start();
    return 0;
}
