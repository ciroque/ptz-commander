#include "ExportCommand.h"
#include "../../cameras/PresetStore.h"
#include <iostream>

namespace commands::setup {
    void ExportCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.size() != 1) {
            ctx.err << "Usage: setup export <file>" << std::endl;
            return;
        }

        cameras::PresetStore store;
        if (!store.save(ctx.cameraMgr, ctx.sceneStore, ctx.viscaIpEndpoints, ctx.viscaSerialEndpoints,
                        ctx.onvifEndpoints, tokens[0], false)) {
            ctx.err << "Failed to write to " << tokens[0] << std::endl;
            return;
        }

        const auto cameraCount = ctx.cameraMgr.getCameras().size();
        ctx.out << "Exported setup to " << tokens[0] << " without ONVIF passwords";
        if (cameraCount > 0) {
            ctx.out << " for " << cameraCount << " camera" << (cameraCount == 1 ? "" : "s");
        }
        ctx.out << std::endl;
    }
} // namespace commands::setup
