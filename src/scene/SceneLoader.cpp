#include "SceneLoader.h"
#include "Scene.h"
#include "common/Log.h"

#include <fstream>
#include <iostream>
#include <string>

#include "third-party/json.h"
using json = nlohmann::json;

#define LOAD_ASSERT(cond, msg) \
    if (!(cond)) { \
        RT_ERROR("Error loading scene: {0}", msg); \
        return false; \
    }

bool LoadPrimitive(class Scene& scene, const json& primitiveData) {
    LOAD_ASSERT(primitiveData.contains("type"), "Primitive must have a 'type' field");
    std::string type = primitiveData["type"];

    if (type == "sphere") {
        LOAD_ASSERT(primitiveData.contains("center"), "Sphere must have a 'center' field");
        LOAD_ASSERT(primitiveData.contains("radius"), "Sphere must have a 'radius' field");
        // Additional loading logic for sphere...
        RT_INFO("Loaded sphere primitive");
    } else if (type == "plane") {
        LOAD_ASSERT(primitiveData.contains("normal"), "Plane must have a 'normal' field");
        LOAD_ASSERT(primitiveData.contains("distance"), "Plane must have a 'distance' field");
        // Additional loading logic for plane...
        RT_INFO("Loaded plane primitive");
    } else {
        RT_ERROR("Unknown primitive type: {0}", type);
        return false;
    }

    return true;
}

bool SceneLoader::LoadScene(class Scene& scene, const std::string& filename) {
    scene.ClearScene();

    std::ifstream f("ex1.json");
    json data = json::parse(f);

    if (data.contains("primitives")) {
        LOAD_ASSERT(data["primitives"].is_array(), "'primitives' must be an array");
        for (const auto& primitiveData : data["primitives"]) {
            LOAD_ASSERT(LoadPrimitive(scene, primitiveData), "Failed to load a primitive");
        }
    }
    
    return true;
}