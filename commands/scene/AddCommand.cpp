#include "AddCommand.h"
#include "../Arguments.h"
#include "../../cameras/Camera.h"
#include <vector>

namespace commands::scene {

    void AddCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 3 || ((tokens.size() - 1) % 2) != 0) {
            ctx.err << "Usage: scene add <name> <id> <preset> [<id> <preset> ...]" << std::endl;
            return;
        }

        const std::string sceneName = tokens[0];

        struct Pair {
            std::string serial;
            std::string presetName;
            std::string id;
        };
        std::vector<Pair> pairs;

        for (size_t i = 1; i < tokens.size(); i += 2) {
            const std::string& id = tokens[i];
            const std::string& presetName = tokens[i + 1];

            if (id == "*" || presetName == "*") {
                ctx.err << "scene add does not accept '*' for camera id or preset name" << std::endl;
                return;
            }

            auto camera = ctx.cameraMgr.findById(id);
            if (!camera) {
                ctx.err << "Camera not found: " << id << std::endl;
                return;
            }
            if (!camera->GetPresetByName(presetName)) {
                ctx.err << "Preset not found: " << presetName
                        << " for " << camera->getSerialNumber() << std::endl;
                return;
            }

            const std::string serial = camera->getSerialNumber();
            for (const auto& existing : pairs) {
                if (existing.serial == serial) {
                    ctx.err << "Duplicate camera in scene add: " << id << std::endl;
                    return;
                }
            }

            pairs.push_back({serial, presetName, id});
        }

        for (const auto& pair : pairs) {
            ctx.sceneStore.upsertBinding(sceneName, pair.serial, pair.presetName);
        }

        ctx.out << "Added " << pairs.size() << " binding"
                << (pairs.size() == 1 ? "" : "s")
                << " to scene '" << sceneName << "'" << std::endl;
    }

}
