#include "ApplyCommand.h"
#include "../Arguments.h"

namespace commands::scene {

    void ApplyCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.empty()) {
            ctx.err << "Usage: scene apply <name>" << std::endl;
            return;
        }

        const std::string& sceneName = tokens[0];
        const auto* scene = ctx.sceneStore.find(sceneName);
        if (!scene) {
            ctx.err << "Scene not found: " << sceneName << std::endl;
            return;
        }

        if (scene->bindings.empty()) {
            ctx.err << "Scene '" << sceneName << "' has no bindings." << std::endl;
            return;
        }

        int applied = 0;
        int skipped = 0;

        for (const auto& [serial, presetName] : scene->bindings) {
            auto camera = ctx.cameraMgr.findBySerial(serial);
            if (!camera) {
                ctx.err << "Camera not connected: " << serial << " (skipping)" << std::endl;
                skipped++;
                continue;
            }

            const cameras::Preset* preset = camera->GetPresetByName(presetName);
            if (!preset) {
                ctx.err << "Preset not found: " << presetName
                        << " for " << serial << " (skipping)" << std::endl;
                skipped++;
                continue;
            }

            if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {
                ctx.err << "Failed to apply '" << presetName << "' to " << serial << std::endl;
                skipped++;
                continue;
            }

            ctx.out << "Applied '" << presetName << "' to " << camera->getFriendlyName() << std::endl;
            applied++;
        }

        ctx.out << "Scene '" << sceneName << "': " << applied << " applied, "
                << skipped << " skipped" << std::endl;
    }

}
