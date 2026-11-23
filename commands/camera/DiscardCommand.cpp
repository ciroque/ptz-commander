#include "DiscardCommand.h"
#include "../Arguments.h"

#include <iostream>
#include <string>

namespace commands::camera {

    namespace {

        std::size_t scenesBinding(const cameras::SceneStore& store, const std::string& serial) {
            std::size_t count = 0;
            for (const auto& entry : store.all()) {
                if (entry.second.bindings.find(serial) != entry.second.bindings.end()) {
                    ++count;
                }
            }
            return count;
        }

        // Only the non-zero parts. "1 preset", "N presets", "a binding in 1 scene",
        // "bindings in M scenes", joined with a comma when both apply.
        std::string lossSummary(std::size_t presets, std::size_t scenes) {
            std::string text;
            if (presets == 1) {
                text = "1 preset";
            } else if (presets > 1) {
                text = std::to_string(presets) + " presets";
            }
            if (scenes > 0) {
                if (!text.empty()) {
                    text += ", ";
                }
                if (scenes == 1) {
                    text += "a binding in 1 scene";
                } else {
                    text += "bindings in " + std::to_string(scenes) + " scenes";
                }
            }
            return text;
        }

    } // namespace

    void DiscardCommand::execute(data::Context& ctx, const std::string& args) {
        auto tokens = commands::splitArgs(args);
        if (tokens.size() != 1) {
            ctx.err << "Usage: camera discard <id>" << std::endl;
            return;
        }
        if (tokens[0] == "*") {
            ctx.err << "camera discard does not accept '*'" << std::endl;
            return;
        }
        auto camera = ctx.cameraMgr.findById(tokens[0]);
        if (!camera) {
            ctx.err << "Camera not found: " << tokens[0] << std::endl;
            return;
        }
        if (!ctx.discardCamera) {
            ctx.err << "camera discard is not available." << std::endl;
            return;
        }

        const auto serial = camera->getSerialNumber();
        // Check before asking. A plugged-in OBSBOT is refused here, and a
        // false commit does not remove the camera.
        const auto preview = ctx.discardCamera(serial, false);
        if (preview == data::Context::DiscardCameraStatus::Obsbot) {
            ctx.err << serial << " is plugged in. Unplug it first." << std::endl;
            return;
        }
        if (preview == data::Context::DiscardCameraStatus::NotFound) {
            ctx.err << "Camera not found: " << tokens[0] << std::endl;
            return;
        }

        const auto presets = camera->getPresets().size();
        const auto scenes = scenesBinding(ctx.sceneStore, serial);
        if (presets > 0 || scenes > 0) {
            ctx.out << lossSummary(presets, scenes) << ". Discard " << tokens[0] << "? [y/N] " << std::flush;
            const auto answer = prompt_ ? prompt_() : std::nullopt;
            if (!answer) {
                ctx.err << "camera discard needs a console." << std::endl;
                return;
            }
            if (*answer != "y" && *answer != "Y") {
                ctx.out << "Discard declined." << std::endl;
                return;
            }
        }

        const auto status = ctx.discardCamera(serial, true);
        if (status == data::Context::DiscardCameraStatus::Obsbot) {
            ctx.err << serial << " is plugged in. Unplug it first." << std::endl;
            return;
        }
        if (status == data::Context::DiscardCameraStatus::NotFound) {
            ctx.err << "Camera not found: " << tokens[0] << std::endl;
            return;
        }

        const auto emptied = ctx.sceneStore.removeCamera(serial);
        ctx.notifySetupChanged();
        ctx.out << "Discarded " << serial << std::endl;
        for (const auto& name : emptied) {
            ctx.out << "Scene '" << name << "' is empty." << std::endl;
        }
    }
} // namespace commands::camera
