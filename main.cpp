
#include <iostream>
#include <string>
#include <map>
#include <memory>
#include <windows.h>

#include "commands/CommandHandler.h"
#include "data/Context.h"

// Placeholder for your DLL header (replace with actual header file)
// #include "DeviceLib.h" // Your DLL's header file

// REPL loop
void runREPL() {
    data::Context ctx;
    commands::CommandHandler handler(ctx);
    std::string input;

    std::cout << "Device Utility REPL (type 'exit' to quit)\n> ";
    while (std::getline(std::cin, input)) {
        if (input == "exit") break;
        if (!input.empty()) {
            handler.execute(ctx, input);
        }
        std::cout << "> ";
    }
}

int main() {
    runREPL();
    return 0;
}