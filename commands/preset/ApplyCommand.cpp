#include "ApplyCommand.h"

namespace commands::preset {
    void ApplyCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: preset apply <serialNumber> <name>" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.size() < 2) {
            std::cout << "Usage: preset apply <serialNumber> <name>" << std::endl;
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];  // TODO: Multi-word names later

        auto camera = ctx.cameraMgr.findBySerialNumber(serialNumber);
        if (!camera) {
            std::cout << "Camera not found: " << serialNumber << std::endl;
            return;
        }

        auto presets = camera->getPresets();
        auto it = std::find_if(presets.begin(), presets.end(),
            [&presetName](const auto& preset) { return preset->name == presetName; });
        if (it == presets.end()) {
            std::cout << "Preset not found: " << presetName << " for " << serialNumber << std::endl;
            return;
        }

        const auto& preset = *it;
        if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {  // Note: pitch vs. tilt swap if needed
            std::cout << "Failed to apply preset '" << presetName << "' to " << serialNumber << std::endl;
            return;
        }

        std::cout << "Applied preset '" << presetName << "' to " << serialNumber << std::endl;
    }
}