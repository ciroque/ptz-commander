#include "LoadCommand.h"
#include "../../cameras/PresetStore.h"
#include <iostream>

namespace commands::preset {
	void LoadCommand::execute(data::Context& ctx, const std::string& args) {
		auto cameras = ctx.cameraMgr.getCameras();
		if (cameras.empty()) {
			ctx.err << "No cameras found to load presets." << std::endl;
			return;
		}

		std::string filename = "presets.ptzc";
		auto tokens = commands::splitArgs(args);
		if (!tokens.empty()) {
			filename = tokens[0];
		}

		cameras::PresetStore store;
		switch (store.load(ctx.cameraMgr, filename)) {
		case cameras::LoadStatus::NotFound:
			ctx.err << "Failed to open " << filename << std::endl;
			return;
		case cameras::LoadStatus::ParseError:
			ctx.err << "Failed to parse " << filename << std::endl;
			return;
		case cameras::LoadStatus::Ok:
			break;
		}

		ctx.out << "Loaded presets from " << filename << std::endl;
	}
}
