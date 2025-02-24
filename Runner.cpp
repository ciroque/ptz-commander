#include "Runner.h"

void Runner::start() {
    running = true;
    std::string input;
    std::cout << StartMessage;
    while (running && std::getline(std::cin, input)) {
        if (input == StopToken) {
            running = false;
        } else if (!input.empty()) {
            commandHandler.execute(context, input);
        }
        std::cout << Prompt;
    }
}
