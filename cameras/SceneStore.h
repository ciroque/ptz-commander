#ifndef CAMERAS_SCENESTORE_H
#define CAMERAS_SCENESTORE_H

#include "Scene.h"
#include <map>
#include <string>

namespace cameras {

    class SceneStore {
    public:
        bool empty() const { return scenes_.empty(); }

        const std::map<std::string, Scene>& all() const { return scenes_; }

        const Scene* find(const std::string& name) const {
            auto it = scenes_.find(name);
            return (it != scenes_.end()) ? &it->second : nullptr;
        }

        void clear() { scenes_.clear(); }

        void replaceAll(std::map<std::string, Scene> scenes) {
            scenes_ = std::move(scenes);
        }

        void upsertBinding(const std::string& sceneName,
                           const std::string& serial,
                           const std::string& presetName) {
            Scene& scene = scenes_[sceneName];
            scene.name = sceneName;
            scene.bindings[serial] = presetName;
        }

        bool removeBinding(const std::string& sceneName, const std::string& serial) {
            auto it = scenes_.find(sceneName);
            if (it == scenes_.end()) {
                return false;
            }
            return it->second.bindings.erase(serial) > 0;
        }

        bool discard(const std::string& sceneName) {
            return scenes_.erase(sceneName) > 0;
        }

    private:
        std::map<std::string, Scene> scenes_;
    };

}

#endif // CAMERAS_SCENESTORE_H
