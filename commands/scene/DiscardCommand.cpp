#include "DiscardCommand.h"
#include "../Arguments.h"

namespace commands::scene {

    void DiscardCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.empty()) {
            ctx.err << "Usage: scene discard <name>" << std::endl;
            return;
        }

        const std::string& sceneName = tokens[0];
        if (!ctx.sceneStore.discard(sceneName)) {
            ctx.err << "Scene not found: " << sceneName << std::endl;
            return;
        }

        ctx.out << "Discarded scene '" << sceneName << "'" << std::endl;
    }

}
