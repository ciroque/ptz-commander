#include "AddCommand.h"
#include "StoreCommand.h"

namespace commands::scene {

    void AddCommand::execute(data::Context& ctx, const std::string& args) {
        ctx.out << "scene add is now scene store" << std::endl;
        StoreCommand store;
        store.execute(ctx, args);
    }

} // namespace commands::scene
