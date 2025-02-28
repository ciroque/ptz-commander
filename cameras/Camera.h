#ifndef CAMERAS_CAMERA_H
#define CAMERAS_CAMERA_H

#include "Preset.h"
#include <string>
#include <list>
#include <map>  // For std::map
#include <memory>

namespace cameras {
    class Camera {
    protected:
        std::map<std::string, std::shared_ptr<Preset>> presets_;  // Key: preset name, Value: Preset

    public:
        virtual ~Camera() = default;

        // Core PTZ methods (still virtual for overrides)
        virtual std::string getSerialNumber() const = 0;
        virtual std::string getName() const = 0;
        virtual bool isConnected() const = 0;
        virtual bool setPosition(float pan, float tilt, int zoom) = 0;
        virtual Ptz getCurrentPtz() const = 0;

        // Preset methods (concrete in base class)
        std::list<std::shared_ptr<Preset>> getPresets() const {
            std::list<std::shared_ptr<Preset>> list;
            for (const auto& pair : presets_) {
                list.push_back(pair.second);
            }
            return list;
        }

        void AddPreset(const std::string& name, Preset preset) {
            presets_[name] = std::make_shared<Preset>(preset);
        }

        const Preset* GetPresetByName(const std::string& name) const {
            auto it = presets_.find(name);
            return (it != presets_.end()) ? it->second.get() : nullptr;
        }
    };
}

#endif // CAMERAS_CAMERA_H