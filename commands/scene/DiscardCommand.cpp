#include "DiscardCommand.h"
#include "../Arguments.h"

#include <iostream>
#include <string>
#include <vector>

namespace commands::scene {

    void DiscardCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.empty()) {
            ctx.err << "Usage: scene discard <name|*>" << std::endl;
            return;
        }

        const std::string& sceneName = tokens[0];
        if (sceneName == "*") {
            if (ctx.sceneStore.empty()) {
                ctx.err << "No scenes found." << std::endl;
                return;
            }
            ctx.out << "Discard every scene? [y/N] " << std::flush;
            const auto answer = prompt_ ? prompt_() : std::nullopt;
            if (!answer) {
                ctx.err << "scene discard * needs a console." << std::endl;
                return;
            }
            if (*answer != "y" && *answer != "Y") {
                ctx.out << "Discard declined." << std::endl;
                return;
            }

            std::vector<std::string> names;
            names.reserve(ctx.sceneStore.all().size());
            for (const auto& entry : ctx.sceneStore.all()) {
                names.push_back(entry.first);
            }
            for (const auto& name : names) {
                ctx.sceneStore.discard(name);
            }
            ctx.notifySetupChanged();
            ctx.out << "Discarded " << names.size() << " scene" << (names.size() == 1 ? "" : "s") << std::endl;
            return;
        }

        if (!ctx.sceneStore.discard(sceneName)) {
            ctx.err << "Scene not found: " << sceneName << std::endl;
            return;
        }

        ctx.notifySetupChanged();
        ctx.out << "Discarded scene '" << sceneName << "'" << std::endl;
    }

} // namespace commands::scene
