#include "BrowseCommand.h"
#include "../../cameras/PresetStore.h"

namespace commands::preset {
    void BrowseCommand::execute(data::Context& ctx, const std::string&) {
        const std::string dir = cameras::PresetStore::homeDirectory();
        const auto files = cameras::PresetStore::listHomeFiles();

        if (files.empty()) {
            ctx.err << "No preset files in " << dir << std::endl;
            return;
        }

        ctx.out << "Preset files in " << dir << ":" << std::endl;
        for (const auto& name : files) {
            ctx.out << "  " << name << std::endl;
        }
    }
}
