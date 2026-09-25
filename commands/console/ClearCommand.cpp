#include "ClearCommand.h"
#include "ptz_version.h"

#include <cstdlib>

namespace commands::console {
    void ClearCommand::execute(data::Context& ctx, const std::string& args) {
        system("cls");
        ctx.out << "PTZ Commander " << PTZ_VERSION_STRING << " (type 'exit' to quit)\n";
    }
}