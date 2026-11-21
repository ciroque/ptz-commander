#include "LoadCommand.h"
#include "../../cameras/PresetStore.h"
#include "../../cameras/onvif/OnvifEndpoint.h"
#include "../../cameras/visca/ViscaIpEndpoint.h"
#include "../../cameras/visca/ViscaSerialEndpoint.h"
#include <iostream>
#include <vector>

namespace commands::setup {

    namespace {

        // Added: the camera is now inactive. AlreadyPresent: leave the camera
        // that is already loaded alone. Failed: it could not be kept.
        enum class SkipResult { Inactive, AlreadyPresent, Failed };

        SkipResult skipOnvif(data::Context& ctx, const cameras::onvif::OnvifEndpoint& endpoint) {
            auto stored = endpoint;
            stored.password.clear();
            stored.needsPassword = false;
            const auto added = ctx.addInactiveOnvif(stored);
            if (added.status == cameras::onvif::OnvifAddStatus::Failed) {
                ctx.err << "Could not keep ONVIF camera " << endpoint.address() << ": " << added.detail << std::endl;
                return SkipResult::Failed;
            }
            if (added.status == cameras::onvif::OnvifAddStatus::AlreadyPresent) {
                return SkipResult::AlreadyPresent;
            }
            ctx.err << "ONVIF camera " << endpoint.user << "@" << endpoint.host << " left inactive" << std::endl;
            return SkipResult::Inactive;
        }

        void noteSkip(SkipResult skip, bool& onvifMissing, bool& onvifInactive) {
            if (skip == SkipResult::Failed) {
                onvifMissing = true;
            } else if (skip == SkipResult::Inactive) {
                onvifInactive = true;
            }
        }

    } // namespace

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
        case cameras::LoadStatus::NotFound: ctx.err << "Failed to open " << filename << std::endl; return;
        case cameras::LoadStatus::ParseError:
            ctx.err << "Failed to parse " << filename << (parseError.empty() ? "" : ": " + parseError) << std::endl;
            return;
        case cameras::LoadStatus::Ok: break;
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
                        << "' for VISCA-IP camera " << ep.host << ". Known: " << cameras::visca::knownProfileNames()
                        << std::endl;
                return;
            }
        }
        if (!staged.viscaIp.empty() && !ctx.loadViscaIp) {
            ctx.err << "Failed to load " << filename << ": VISCA-IP is not available." << std::endl;
            return;
        }
        for (const auto& ep : staged.viscaSerial) {
            if (!cameras::visca::resolveProfile(ep)) {
                ctx.err << "Failed to load " << filename << ": unknown profile '" << ep.profile
                        << "' for serial camera " << cameras::visca::serialCameraId(ep)
                        << ". Known: " << cameras::visca::knownProfileNames() << std::endl;
                return;
            }
            // One camera per port, and a camera already added on this port
            // isn't one the file replaces.
            for (const auto& added : ctx.viscaSerialEndpoints) {
                if (added.port == ep.port && added.address != ep.address) {
                    ctx.err << "Failed to load " << filename << ": " << ep.port << " already has camera "
                            << cameras::visca::serialCameraId(added) << ", and the file has "
                            << cameras::visca::serialCameraId(ep) << " (one camera per port)" << std::endl;
                    return;
                }
            }
        }
        if (!staged.viscaSerial.empty() && !ctx.loadViscaSerial) {
            ctx.err << "Failed to load " << filename << ": VISCA serial is not available." << std::endl;
            return;
        }
        for (const auto& ep : staged.onvif) {
            if (ep.host.empty() || ep.user.empty()) {
                ctx.err << "Failed to load " << filename << ": an onvif entry needs a host and a user" << std::endl;
                return;
            }
        }
        if (!staged.onvif.empty() && (!ctx.addOnvif || !ctx.addInactiveOnvif)) {
            ctx.err << "Failed to load " << filename << ": ONVIF is not available." << std::endl;
            return;
        }

        // VISCA cameras are added (or moved onto the file's profile) first,
        // so their presets below are applied under the profile they were saved with.
        for (const auto& ep : staged.viscaIp) {
            if (!ctx.loadViscaIp(ep)) {
                // Checked above, so this is unexpected; earlier entries may
                // already have been applied.
                ctx.err << "Failed to load VISCA-IP camera " << ep.host << "; setup partially loaded" << std::endl;
                return;
            }
        }
        for (const auto& ep : staged.viscaSerial) {
            if (!ctx.loadViscaSerial(ep)) {
                ctx.err << "Failed to load serial camera " << cameras::visca::serialCameraId(ep)
                        << "; setup partially loaded" << std::endl;
                return;
            }
        }
        // A camera that does not answer is left out. A missing password, or a
        // dpapi: blob this user cannot decrypt, is asked for. Skipping keeps
        // the camera inactive so its presets and scene bindings survive.
        // An auth fault asks again, up to three tries. Anything else (the
        // camera is off, it has no PTZ profile) is left out, as before.
        bool onvifMissing = false;
        bool onvifInactive = false;
        for (const auto& ep : staged.onvif) {
            if (!ep.needsPassword) {
                const auto added = ctx.addOnvif(ep);
                if (added.status == cameras::onvif::OnvifAddStatus::Failed) {
                    ctx.err << "Could not add ONVIF camera " << ep.address() << ": " << added.detail << std::endl;
                    onvifMissing = true;
                }
                continue;
            }

            bool settled = false;
            for (int attempt = 0; attempt < 3; ++attempt) {
                const auto password = prompt_ ? prompt_(ep) : std::nullopt;
                if (!password || password->empty()) {
                    noteSkip(skipOnvif(ctx, ep), onvifMissing, onvifInactive);
                    settled = true;
                    break;
                }

                auto withPassword = ep;
                withPassword.password = *password;
                withPassword.needsPassword = false;
                const auto added = ctx.addOnvif(withPassword);
                if (added.status != cameras::onvif::OnvifAddStatus::Failed) {
                    settled = true;
                    break;
                }
                if (!added.authRejected) {
                    ctx.err << "Could not add ONVIF camera " << ep.address() << ": " << added.detail << std::endl;
                    onvifMissing = true;
                    settled = true;
                    break;
                }
                ctx.err << "ONVIF password rejected for " << ep.user << "@" << ep.host;
                if (attempt + 1 < 3) {
                    ctx.err << "; try again";
                }
                ctx.err << std::endl;
            }
            if (!settled) {
                noteSkip(skipOnvif(ctx, ep), onvifMissing, onvifInactive);
            }
        }
        store.apply(ctx.cameraMgr, ctx.sceneStore, staged);

        ctx.notifySetupChanged();
        ctx.out << "Loaded setup from " << filename;
        if (onvifMissing && onvifInactive) {
            ctx.out << " (an ONVIF camera did not answer, and one was left inactive)";
        } else if (onvifMissing) {
            ctx.out << " (an ONVIF camera did not answer and was not added)";
        } else if (onvifInactive) {
            ctx.out << " (an ONVIF camera was left inactive)";
        }
        ctx.out << std::endl;
    }
} // namespace commands::setup
