#include "ApplyCommand.h"
#include "../ConnectionLabel.h"
#include "../ResolveCameras.h"
#include "../TourRunner.h"
#include <chrono>
#include <iostream>
#include <iterator>
#include <memory>
#include <thread>

namespace commands::preset {
    namespace {
        constexpr int kDefaultDelayMs = 2000;

        void printUsage(data::Context& ctx) { ctx.err << "Usage: preset apply <id|*> <name>" << std::endl; }

        // The summary prints after the last move. Empty cameras and inactive
        // cameras are reported here, before the runner is started.
        void startPresetCycle(data::Context& ctx, const std::string& serialNumber, int delayMs) {
            ctx.tour->stop();
            auto cameras = commands::resolveCameras(ctx, serialNumber);
            if (cameras.empty()) {
                return;
            }

            auto allGood = std::make_shared<bool>(true);
            std::vector<commands::TourRunner::Step> steps;
            for (auto& camera : cameras) {
                if (commands::refuseNeedsLogin(ctx, *camera)) {
                    *allGood = false;
                    continue;
                }
                auto presets = camera->getPresets();
                if (presets.empty()) {
                    ctx.err << "No presets found for " << camera->getSerialNumber() << std::endl;
                    *allGood = false;
                    continue;
                }

                for (const auto& preset : presets) {
                    auto cam = camera;
                    auto presetCopy = preset;
                    steps.push_back([cam, presetCopy, allGood](data::Context& stepCtx) {
                        if (!cam->setPosition(presetCopy.ptz.pan, presetCopy.ptz.tilt, presetCopy.ptz.zoom)) {
                            stepCtx.err << "Failed to apply preset '" << presetCopy.name << "' to "
                                        << cam->getSerialNumber() << std::endl;
                            *allGood = false;
                        } else {
                            stepCtx.out << "Applied preset '" << presetCopy.name << "' to " << cam->getSerialNumber()
                                        << std::endl;
                        }
                    });
                }
            }

            const std::string target = serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber;
            auto finish = [allGood, target](data::Context& stepCtx) {
                if (*allGood) {
                    stepCtx.out << "Applied presets to " << target << std::endl;
                } else {
                    stepCtx.err << "Some presets failed to apply for " << target << "." << std::endl;
                }
            };
            if (steps.empty()) {
                finish(ctx);
                return;
            }

            ctx.tour->start("preset apply " + serialNumber + " *", std::move(steps), std::chrono::milliseconds(delayMs),
                            {}, std::move(finish), ctx);
        }
    } // namespace

    void ApplyCommand::execute(data::Context& ctx, const std::string& args) {
        if (args.empty()) {
            printUsage(ctx);
            return;
        }

        auto tokens = commands::splitArgs(args);
        if (tokens.size() < 2) {
            printUsage(ctx);
            return;
        }

        std::string serialNumber = tokens[0];
        std::string presetName = tokens[1];
        int delayMs = kDefaultDelayMs;

        // One release: * still cycles in milliseconds and names preset tour.
        const bool cycle = presetName == "*";
        if (cycle) {
            ctx.out << "use preset tour " << serialNumber << " [seconds]" << std::endl;
            if (tokens.size() > 2) {
                try {
                    delayMs = std::stoi(tokens[2]);
                    if (delayMs < 0) {
                        ctx.err << "Invalid delay_ms, using default " << kDefaultDelayMs << "ms" << std::endl;
                        delayMs = kDefaultDelayMs;
                    }
                } catch (const std::exception&) {
                    ctx.err << "Invalid delay_ms, using default " << kDefaultDelayMs << "ms" << std::endl;
                    delayMs = kDefaultDelayMs;
                }
            }
        } else if (tokens.size() > 2) {
            printUsage(ctx);
            return;
        }

        if (cycle && ctx.tour) {
            startPresetCycle(ctx, serialNumber, delayMs);
            return;
        }
        if (!cycle && ctx.tour) {
            ctx.tour->stop();
        }

        auto cameras = commands::resolveCameras(ctx, serialNumber);
        if (cameras.empty()) {
            return;
        }

        bool allGood = true;
        for (auto& camera : cameras) {
            if (commands::refuseNeedsLogin(ctx, *camera)) {
                allGood = false;
                continue;
            }
            if (cycle) {
                // Cycle through all presets for this camera
                auto presets = camera->getPresets();
                if (presets.empty()) {
                    ctx.err << "No presets found for " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                    continue;
                }

                for (auto it = presets.begin(); it != presets.end(); ++it) {
                    const auto& preset = *it;
                    if (!camera->setPosition(preset.ptz.pan, preset.ptz.tilt, preset.ptz.zoom)) {
                        ctx.err << "Failed to apply preset '" << preset.name << "' to " << camera->getSerialNumber()
                                << std::endl;
                        allGood = false;
                    } else {
                        ctx.out << "Applied preset '" << preset.name << "' to " << camera->getSerialNumber()
                                << std::endl;
                    }
                    if (std::next(it) != presets.end() && delayMs > 0) {
                        std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                    }
                }
            } else {
                // Apply single preset by name
                const cameras::Preset* preset = camera->getPresetByName(presetName);
                if (!preset) {
                    ctx.err << "Preset not found: " << presetName << " for " << camera->getSerialNumber() << std::endl;
                    allGood = false;
                    continue;
                }

                if (!camera->setPosition(preset->ptz.pan, preset->ptz.tilt, preset->ptz.zoom)) {
                    ctx.err << "Failed to apply preset '" << presetName << "' to " << camera->getSerialNumber()
                            << std::endl;
                    allGood = false;
                } else {
                    ctx.out << "Applied preset '" << presetName << "' to " << camera->getSerialNumber() << std::endl;
                }
            }
        }

        const std::string target = serialNumber == "*" ? std::to_string(cameras.size()) + " cameras" : serialNumber;
        if (allGood) {
            if (cycle) {
                ctx.out << "Applied presets to " << target << std::endl;
            } else {
                ctx.out << "Applied preset '" << presetName << "' to " << target << std::endl;
            }
        } else {
            ctx.err << "Some presets failed to apply for " << target << "." << std::endl;
        }
    }
} // namespace commands::preset
