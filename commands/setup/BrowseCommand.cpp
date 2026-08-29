#include "BrowseCommand.h"
#include "../../cameras/PresetStore.h"

namespace commands::setup {
    void BrowseCommand::execute(data::Context& ctx, const std::string&) {
        const std::string dir = cameras::PresetStore::homeDirectory();
        const auto files = cameras::PresetStore::listHomeFiles();

        if (files.empty()) {
            ctx.err << "No setup files in " << dir << std::endl;
            return;
        }

        ctx.out << "Setup files in " << dir << ":" << std::endl;
        for (const auto& name : files) {
            ctx.out << "  " << name << std::endl;
        }
    }
}
