cmake_minimum_required(VERSION 3.30)
project(ptz_command)

set(CMAKE_CXX_STANDARD 20)

add_executable(ptz_command main.cpp
        commands/Command.cpp
        commands/Command.h
        commands/ListCommand.cpp
        commands/ListCommand.h
        data/Context.cpp
        data/Context.h
        commands/ShowCommand.cpp
        commands/ShowCommand.h
        commands/SelectCommand.cpp
        commands/SelectCommand.h
        commands/CommandHandler.cpp
        commands/CommandHandler.h
        Runner.cpp
        Runner.h)
