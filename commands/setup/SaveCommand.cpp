#include "SaveCommand.h"
#include "../../cameras/PresetStore.h"
#include <iostream>

namespace commands::setup {
    void SaveCommand::execute(data::Context& ctx, const std::string& args) {
        std::string filename = "presets.ptzc";
        auto tokens = commands::splitArgs(args);
        if (!tokens.empty()) {
            filename = tokens[0];
        }

        cameras::PresetStore store;
        if (!store.save(ctx.cameraMgr, ctx.sceneStore, filename)) {
            ctx.err << "Failed to write to " << filename << std::endl;
            return;
        }

        const auto cameraCount = ctx.cameraMgr.getCameras().size();
        ctx.out << "Saved setup to " << filename;
        if (cameraCount > 0) {
            ctx.out << " for " << cameraCount << " camera" << (cameraCount == 1 ? "" : "s");
        }
        ctx.out << std::endl;
    }
}
