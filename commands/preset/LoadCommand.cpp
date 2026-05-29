#include "LoadCommand.h"
#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

namespace commands::preset {
	void LoadCommand::execute(data::Context& ctx, const std::string& args) {
		auto cameras = ctx.cameraMgr.getCameras();
		if (cameras.empty()) {
			std::cout << "No cameras found to load presets." << std::endl;
			return;
		}

		std::string filename = "presets.ptzc";
		auto tokens = splitArgs(args);
		if (!tokens.empty()) {
			filename = tokens[0];
			if (filename.find('.') == std::string::npos) {
				filename += ".ptzc";
			}
		}

		std::ifstream file(filename);
		if (!file.is_open()) {
			std::cout << "Failed to open " << filename << std::endl;
			return;
		}

		try {
			nlohmann::json j;
			file >> j;
			file.close();

			int loadedCount = 0;
			for (auto& camera : cameras) {
				std::string sn = camera->getSerialNumber();
				if (j.contains(sn)) {
					auto camJson = j[sn];
					for (auto it = camJson.begin(); it != camJson.end(); ++it) {
						std::string presetName = it.key();
						auto presetJson = it.value();
						cameras::Preset preset;
						preset.name = presetName;
						preset.ptz.pan = presetJson["pan"].get<float>();
						preset.ptz.tilt = presetJson["tilt"].get<float>();
						preset.ptz.zoom = presetJson["zoom"].get<int>();
						camera->AddPreset(presetName, preset);
						loadedCount++;
					}
				}
			}

			std::cout << "Loaded " << loadedCount << " presets from " << filename << " for "
				<< cameras.size() << " cameras" << std::endl;
		}
		catch (const std::exception& e) {
			std::cout << "Error loading " << filename << ": " << e.what() << std::endl;
		}
	}
}
