#ifndef RUNNER_H
#define RUNNER_H
#include "commands/Command.h"
#include "commands/CommandHandler.h"
#include "data/Context.h"



class Runner {
private:
    data::Context context;
    commands::CommandHandler commandHandler;
    bool running;

    const std::string StartMessage = "PTZ Commander (type 'exit' to quit)\n> ";
    const std::string Prompt = "> ";
    const std::string StopToken = "exit";

public:
    Runner() : commandHandler(context), running(false) { }

    void start();
};



#endif //RUNNER_H
