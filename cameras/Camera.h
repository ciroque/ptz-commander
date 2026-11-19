#ifndef CAMERAS_CAMERA_H
#define CAMERAS_CAMERA_H

#include "Preset.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace cameras {
    class Camera {
    protected:
        std::map<std::string, Preset> presets_;
        std::string alias_;

    public:
        virtual ~Camera() = default;

        std::string getAlias() const { return alias_; }
        void setAlias(const std::string& alias) { alias_ = alias; }

        std::string getFriendlyName() const { return alias_.empty() ? getName() : alias_; }

        // Core PTZ methods (still virtual for overrides)
        virtual std::string getSerialNumber() const = 0;
        virtual std::string getName() const = 0;
        virtual bool isConnected() const = 0;
        virtual bool setPosition(float pan, float tilt, int zoom) = 0;
        virtual bool setZoom(int zoom, int speed) = 0;
        virtual Ptz getCurrentPtz() const = 0;

        // Preset methods (concrete in base class). Presets are values: a
        // name plus a pan/tilt/zoom, with no shared ownership.
        std::vector<Preset> getPresets() const {
            std::vector<Preset> list;
            list.reserve(presets_.size());
            for (const auto& pair : presets_) {
                list.push_back(pair.second);
            }
            return list;
        }

        void addPreset(const std::string& name, Preset preset) {
            presets_.insert_or_assign(name, std::move(preset));
        }

        const Preset* getPresetByName(const std::string& name) const {
            auto it = presets_.find(name);
            return (it != presets_.end()) ? &it->second : nullptr;
        }

        void removePreset(const std::string& name) {
            presets_.erase(name);
        }

        void clearPresets() {
            presets_.clear();
        }
    };
} // namespace cameras

#endif // CAMERAS_CAMERA_H
