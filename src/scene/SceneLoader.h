#ifndef SCENE_LOADER_H
#define SCENE_LOADER_H

#include <string>

class SceneLoader {
public:
    static bool LoadScene(class Scene& scene, const std::string& filename);
    static bool HotReloadSceneIfNeeded(class Scene& scene);

    inline static bool s_LoadCameraSettings = true; 
};

#endif