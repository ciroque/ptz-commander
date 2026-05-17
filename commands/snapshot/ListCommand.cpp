#include "ListCommand.h"
#include "../../cameras/Camera.h"
#include <iostream>
#include <map>

namespace commands::snapshot {

    void ListCommand::execute(data::Context& ctx, const std::string& args) {
        auto cameras = ctx.cameraMgr.getCameras();

        if (cameras.empty()) {
            std::cout << "No cameras found." << std::endl;
            return;
        }

        // Map of preset name -> count of cameras that have it
        std::map<std::string, int> presetCounts;

        // Collect all preset names from all cameras
        for (const auto& camera : cameras) {
            auto presets = camera->getPresets();
            for (const auto& preset : presets) {
                presetCounts[preset->name]++;
            }
        }

        if (presetCounts.empty()) {
            std::cout << "No presets found on any camera." << std::endl;
            return;
        }

        std::cout << "Snapshot presets available:" << std::endl;
        for (const auto& [name, count] : presetCounts) {
            std::cout << "  " << name << "  (" << count << " camera" 
                      << (count == 1 ? "" : "s") << ")" << std::endl;
        }
    }

} // namespace commands::snapshot
