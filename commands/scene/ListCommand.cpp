#include "ListCommand.h"

namespace commands::scene {

    void ListCommand::execute(data::Context& ctx, const std::string&) {
        const auto& scenes = ctx.sceneStore.all();
        if (scenes.empty()) {
            ctx.err << "No scenes found." << std::endl;
            return;
        }

        ctx.out << "Scenes:" << std::endl;
        for (const auto& [name, scene] : scenes) {
            const auto count = scene.bindings.size();
            ctx.out << "  " << name << "  (" << count << " binding"
                    << (count == 1 ? "" : "s") << ")" << std::endl;
        }
    }

}
