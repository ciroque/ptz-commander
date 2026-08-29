#include "ShowCommand.h"
#include "../Arguments.h"
#include <iomanip>

namespace commands::scene {

    void ShowCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.empty()) {
            ctx.err << "Usage: scene show <name>" << std::endl;
            return;
        }

        const std::string& sceneName = tokens[0];
        const auto* scene = ctx.sceneStore.find(sceneName);
        if (!scene) {
            ctx.err << "Scene not found: " << sceneName << std::endl;
            return;
        }

        ctx.out << "Scene: " << sceneName << std::endl;
        if (scene->bindings.empty()) {
            ctx.out << "  (no bindings)" << std::endl;
            return;
        }

        ctx.out << std::left
                << std::setw(20) << "Camera" << " | "
                << std::setw(24) << "Serial" << " | "
                << "Preset" << std::endl;
        ctx.out << std::string(20, '-') << "-+-"
                << std::string(24, '-') << "-+-"
                << std::string(12, '-') << std::endl;

        for (const auto& [serial, presetName] : scene->bindings) {
            auto camera = ctx.cameraMgr.findBySerial(serial);
            const std::string label = camera ? camera->getFriendlyName() : "(not connected)";
            ctx.out << std::left
                    << std::setw(20) << label << " | "
                    << std::setw(24) << serial << " | "
                    << presetName << std::endl;
        }
    }

}
