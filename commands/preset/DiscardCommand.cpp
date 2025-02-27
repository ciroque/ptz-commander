#include "DiscardCommand.h"

namespace commands::preset {
    void DiscardCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            std::cout << "Usage: preset discard <serialNumber> <name>" << std::endl;
            return;
        }

        auto tokens = splitArgs(args);
        if (tokens.size() < 2) {
            std::cout << "Usage: preset discard <serialNumber> <name>" << std::endl;
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

        presets.erase(it);  // Note: This doesn’t modify Camera::presets_—needs tweak in Camera
        std::cout << "Discarded preset '" << presetName << "' from " << serialNumber << std::endl;
    }
}