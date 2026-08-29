#include "SaveCommand.h"
#include "../../cameras/PresetStore.h"
#include <iostream>

namespace commands::preset {
    void SaveCommand::execute(data::Context& ctx, const std::string& args) {
        auto cameras = ctx.cameraMgr.getCameras();
        if (cameras.empty()) {
            ctx.err << "No cameras found to save presets." << std::endl;
            return;
        }

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

        ctx.out << "Saved presets to " << filename << " for " << cameras.size() << " cameras" << std::endl;
    }
}