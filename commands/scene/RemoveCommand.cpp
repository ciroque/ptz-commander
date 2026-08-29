#include "RemoveCommand.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"

namespace commands::scene {

    void RemoveCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2) {
            ctx.err << "Usage: scene remove <name> <id>" << std::endl;
            return;
        }

        const std::string& sceneName = tokens[0];
        const std::string& id = tokens[1];

        if (!ctx.sceneStore.find(sceneName)) {
            ctx.err << "Scene not found: " << sceneName << std::endl;
            return;
        }

        auto camera = ctx.cameraMgr.findById(id);
        const std::string serial = camera ? camera->getSerialNumber() : id;

        if (!ctx.sceneStore.removeBinding(sceneName, serial)) {
            ctx.err << "Binding not found: " << id << " in scene '" << sceneName << "'" << std::endl;
            return;
        }

        ctx.out << "Removed " << serial << " from scene '" << sceneName << "'" << std::endl;
    }

}
