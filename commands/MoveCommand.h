#ifndef MOVECOMMAND_H
#define MOVECOMMAND_H

#include "Command.h"

namespace commands {
	class MoveCommand : public Command {
	public:
		MoveCommand() { name = "move"; }
		void execute(data::Context& ctx, const std::string& args) override;
	};
}

#endif