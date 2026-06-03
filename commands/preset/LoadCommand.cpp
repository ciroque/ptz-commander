#include "LoadCommand.h"
#include "../../cameras/PresetStore.h"
#include <iostream>

namespace commands::preset {
	void LoadCommand::execute(data::Context& ctx, const std::string& args) {
		auto cameras = ctx.cameraMgr.getCameras();
		if (cameras.empty()) {
			std::cout << "No cameras found to load presets." << std::endl;
			return;
		}

		std::string filename = "presets.ptzc";
		auto tokens = commands::splitArgs(args);
		if (!tokens.empty()) {
			filename = tokens[0];
		}
		filename = cameras::PresetStore::normalizeFilename(filename);

		cameras::PresetStore store;
		if (!store.load(ctx.cameraMgr, filename)) {
			std::cout << "Failed to open " << filename << std::endl;
			return;
		}

		std::cout << "Loaded presets from " << filename << " for "
			<< cameras.size() << " cameras" << std::endl;
	}
}
