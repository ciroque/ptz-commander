#include "MoveCommand.h"

#include "MoveCommand.h"

#include <iostream>

void commands::MoveCommand::execute(data::Context& ctx, const std::string& args)
{
	std::cout << "MoveCommand::execute" << args << std::endl;
}
