#include "SceneLoader.h"
#include "Scene.h"
#include "common/Log.h"

#include "primitives/Sphere.h"
#include "primitives/Triangle.h"
#include "primitives/InfinitePlane.h"
#include "primitives/Box.h"
#include "shaders/FlatShader.h"

#include <fstream>
#include <iostream>
#include <string>

#include "third-party/json.h"
using json = nlohmann::json;

extern UBO uniformBufferData;

#define LOAD_ASSERT(cond, msg) \
    if (!(cond)) { \
        RT_ERROR("Error loading scene: {0}", msg); \
        return false; \
    }

static std::unordered_map<std::string, std::shared_ptr<Shader>> s_Shaders;


bool LoadSettings(class Scene& scene, const json& settings) {
    // Currently no settings to load
    if (settings.contains("camera")) {
        LOAD_ASSERT(settings["camera"].is_object(), "'camera' must be an object");
        if (settings["camera"].contains("position")) {
            LOAD_ASSERT(settings["camera"]["position"].is_array() && settings["camera"]["position"].size() == 3, "'camera.position' must be an array of 3 elements");
            glm::vec3 position = glm::vec3(settings["camera"]["position"][0], settings["camera"]["position"][1], settings["camera"]["position"][2]);
            uniformBufferData.u_CameraPosition = glm::vec4(position, 0.0f);
        }
    }

    return true;
}

bool LoadShader(class Scene& scene, const json& shader) {
    LOAD_ASSERT(shader.contains("type"), "Shader must have a 'type' field");
    LOAD_ASSERT(shader.contains("name"), "Shader must have a 'name' field");
    std::string type = shader["type"];
    std::string name = shader["name"];
    std::shared_ptr<Shader> shaderPtr = nullptr;

    if (type == "flat") {
        LOAD_ASSERT(shader.contains("color"), "FlatShader must have a 'color' field");
        glm::vec3 color = glm::vec3(shader["color"][0], shader["color"][1], shader["color"][2]);
        shaderPtr = std::make_shared<FlatShader>(color);
    }

    LOAD_ASSERT(shaderPtr, "Unknown shader type: " + type);
    scene.AddShader(shaderPtr);
    s_Shaders[name] = shaderPtr;
    return true;
}

bool LoadPrimitive(class Scene& scene, const json& primitiveData) {
    LOAD_ASSERT(primitiveData.contains("type"), "Primitive must have a 'type' field");
    LOAD_ASSERT(primitiveData.contains("shader"), "Primitive must have a 'shader' field");
    std::string type = primitiveData["type"];
    std::string shaderName = primitiveData["shader"];
    LOAD_ASSERT(s_Shaders.find(shaderName) != s_Shaders.end(), "Shader not found: " + shaderName);

    if (type == "sphere") {
        LOAD_ASSERT(primitiveData.contains("center"), "Sphere must have a 'center' field");
        LOAD_ASSERT(primitiveData.contains("radius"), "Sphere must have a 'radius' field");
        std::shared_ptr<Sphere> sphere = std::make_shared<Sphere>(
            Vec3(primitiveData["center"][0], primitiveData["center"][1], primitiveData["center"][2]),
            primitiveData["radius"],
            s_Shaders[shaderName]
        );
        scene.AddPrimitive(sphere);
    } else if (type == "triangle") {
        LOAD_ASSERT(primitiveData.contains("vertices"), "Triangle must have a 'vertices' field");
        std::shared_ptr<Triangle> triangle = std::make_shared<Triangle>(
            Vec3(primitiveData["vertices"][0][0], primitiveData["vertices"][0][1], primitiveData["vertices"][0][2]),
            Vec3(primitiveData["vertices"][1][0], primitiveData["vertices"][1][1], primitiveData["vertices"][1][2]),
            Vec3(primitiveData["vertices"][2][0], primitiveData["vertices"][2][1], primitiveData["vertices"][2][2]),
            s_Shaders[shaderName]
        );
        scene.AddPrimitive(triangle);
    } else if (type == "plane") {
        LOAD_ASSERT(primitiveData.contains("origin"), "Plane must have an 'origin' field");
        LOAD_ASSERT(primitiveData.contains("normal"), "Plane must have a 'normal' field");
        std::shared_ptr<InfinitePlane> plane = std::make_shared<InfinitePlane>(
            Vec3(primitiveData["origin"][0], primitiveData["origin"][1], primitiveData["origin"][2]),
            Vec3(primitiveData["normal"][0], primitiveData["normal"][1], primitiveData["normal"][2]),
            s_Shaders[shaderName]
        );
        scene.AddPrimitive(plane);
    } else if (type == "box") {
        LOAD_ASSERT(primitiveData.contains("center"), "Box must have a 'center' field");
        LOAD_ASSERT(primitiveData.contains("size"), "Box must have a 'size' field");
        std::shared_ptr<Box> box = std::make_shared<Box>(
            Vec3(primitiveData["center"][0], primitiveData["center"][1], primitiveData["center"][2]),
            Vec3(primitiveData["size"][0], primitiveData["size"][1], primitiveData["size"][2]),
            s_Shaders[shaderName]
        );
        scene.AddPrimitive(box);
    } else {
        RT_ERROR("Unknown primitive type: {0}", type);
        return false;
    }

    return true;
}

bool SceneLoader::LoadScene(class Scene& scene, const std::string& filename) {
    scene.ClearScene();
    s_Shaders.clear();

    std::ifstream f("ex1.json");
    json data = json::parse(f);

    if (data.contains("settings")) {
        LOAD_ASSERT(data["settings"].is_object(), "'settings' must be an object");
        LOAD_ASSERT(LoadSettings(scene, data["settings"]), "Failed to load settings");
    }

    if (data.contains("shaders")) {
        LOAD_ASSERT(data["shaders"].is_array(), "'shaders' must be an array");
        for (const auto& shader : data["shaders"]) {
            LOAD_ASSERT(LoadShader(scene, shader), "Failed to load a shader");
        }
    }

    if (data.contains("primitives")) {
        LOAD_ASSERT(data["primitives"].is_array(), "'primitives' must be an array");
        for (const auto& primitiveData : data["primitives"]) {
            LOAD_ASSERT(LoadPrimitive(scene, primitiveData), "Failed to load a primitive");
        }
    }
    
    return true;
}