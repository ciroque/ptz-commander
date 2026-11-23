#include "ShowCommand.h"
#include "../Arguments.h"
#include "../ResolveCameras.h"
#include <iomanip>
#include <vector>

namespace commands::preset {
    namespace {

        void printPresets(data::Context& ctx, const cameras::Camera& camera,
                          const std::vector<cameras::Preset>& presets) {
            ctx.out << "Camera: " << camera.getSerialNumber() << " (" << camera.getFriendlyName() << ")" << std::endl;
            ctx.out << std::left << std::setw(20) << "Preset Name" << " | " << std::setw(10) << "Pan" << " | "
                    << std::setw(10) << "Tilt" << " | " << std::setw(10) << "Zoom" << std::endl;
            ctx.out << std::string(20, '-') << "-+-" << std::string(10, '-') << "-+-" << std::string(10, '-') << "-+-"
                    << std::string(10, '-') << std::endl;

            for (const auto& preset : presets) {
                ctx.out << std::left << std::setw(20) << preset.name << " | " << std::setw(10) << preset.ptz.pan
                        << " | " << std::setw(10) << preset.ptz.tilt << " | " << std::setw(10) << preset.ptz.zoom
                        << std::endl;
            }
            ctx.out << std::endl;
        }

    } // namespace

    void ShowCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.empty() || tokens.size() > 2) {
            ctx.err << "Usage: preset show <id|*> [name]" << std::endl;
            return;
        }

        auto cameras = commands::resolveCameras(ctx, tokens[0]);
        if (cameras.empty()) {
            return;
        }

        const bool named = tokens.size() == 2;
        bool any = false;
        for (const auto& camera : cameras) {
            if (!named) {
                auto presets = camera->getPresets();
                if (presets.empty()) {
                    continue;
                }
                printPresets(ctx, *camera, presets);
                any = true;
                continue;
            }

            const cameras::Preset* preset = camera->getPresetByName(tokens[1]);
            if (!preset) {
                ctx.err << "Preset not found: " << tokens[1] << " for " << camera->getSerialNumber() << std::endl;
                continue;
            }
            printPresets(ctx, *camera, {*preset});
            any = true;
        }

        if (!named && !any) {
            ctx.err << "No presets found." << std::endl;
        }
    }

} // namespace commands::preset
