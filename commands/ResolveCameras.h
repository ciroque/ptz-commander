#ifndef COMMANDS_RESOLVE_CAMERAS_H
#define COMMANDS_RESOLVE_CAMERAS_H

#include "../data/Context.h"
#include <list>
#include <memory>
#include <string>

namespace commands {

    /**
     * Resolves <id|*> to a camera list.
     * Prints "No cameras found." or "Camera not found: <id>" and returns empty on failure.
     */
    inline std::list<std::shared_ptr<cameras::Camera>> resolveCameras(
        data::Context& ctx, const std::string& id) {
        if (id == "*") {
            auto cameras = ctx.cameraMgr.getCameras();
            if (cameras.empty()) {
                ctx.err << "No cameras found." << std::endl;
            }
            return cameras;
        }

        auto camera = ctx.cameraMgr.findById(id);
        if (!camera) {
            ctx.err << "Camera not found: " << id << std::endl;
            return {};
        }
        return {std::move(camera)};
    }

} // namespace commands

#endif // COMMANDS_RESOLVE_CAMERAS_H
