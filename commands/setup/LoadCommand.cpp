#include "LoadCommand.h"
#include "../../cameras/PresetStore.h"
#include "../../cameras/visca/ViscaIpEndpoint.h"
#include <iostream>
#include <vector>

namespace commands::setup {
    void LoadCommand::execute(data::Context& ctx, const std::string& args) {
        std::string filename = "presets.ptzc";
        auto tokens = commands::splitArgs(args);
        if (!tokens.empty()) {
            filename = tokens[0];
        }

        // Parse everything before changing anything: switching a VISCA-IP
        // camera's profile and then hitting a bad preset would leave a mix of
        // the old setup and the new.
        cameras::PresetStore store;
        cameras::StagedSetup staged;
        std::string parseError;
        switch (store.read(filename, staged, &parseError)) {
        case cameras::LoadStatus::NotFound:
            ctx.err << "Failed to open " << filename << std::endl;
            return;
        case cameras::LoadStatus::ParseError:
            ctx.err << "Failed to parse " << filename
                    << (parseError.empty() ? "" : ": " + parseError) << std::endl;
            return;
        case cameras::LoadStatus::Ok:
            break;
        }

        // Every VISCA-IP entry must be usable before any is applied; otherwise
        // its presets would land under whatever profile the camera had, or be
        // dropped with the camera.
        for (const auto& ep : staged.viscaIp) {
            if (ep.host.empty()) {
                ctx.err << "Failed to load " << filename << ": a visca_ip entry has no host" << std::endl;
                return;
            }
            if (!cameras::visca::resolveProfile(ep)) {
                ctx.err << "Failed to load " << filename << ": unknown profile '" << ep.profile
                        << "' for VISCA-IP camera " << ep.host << ". Known: "
                        << cameras::visca::knownProfileNames() << std::endl;
                return;
            }
        }
        if (!staged.viscaIp.empty() && !ctx.loadViscaIp) {
            ctx.err << "Failed to load " << filename << ": VISCA-IP is not available." << std::endl;
            return;
        }

        // VISCA-IP cameras are added (or moved onto the file's profile) first,
        // so their presets below are applied under the profile they were saved with.
        for (const auto& ep : staged.viscaIp) {
            if (!ctx.loadViscaIp(ep)) {
                // Checked above, so this is unexpected; earlier entries may
                // already have been applied.
                ctx.err << "Failed to load VISCA-IP camera " << ep.host << "; setup partially loaded" << std::endl;
                return;
            }
        }
        store.apply(ctx.cameraMgr, ctx.sceneStore, staged);

        ctx.notifySetupChanged();
        ctx.out << "Loaded setup from " << filename << std::endl;
    }
}
