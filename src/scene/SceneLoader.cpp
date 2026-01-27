#include "SceneLoader.h"
#include "Scene.h"
#include "common/Log.h"
#include "common/Params.h"
#include "scene/Camera.h"

#include "vulkan/Texture.h"
#include "primitives/Sphere.h"
#include "primitives/Triangle.h"
#include "primitives/InfinitePlane.h"
#include "primitives/Box.h"
#include "primitives/Mesh.h"
#include "shaders/FlatShader.h"
#include "shaders/MirrorShader.h"
#include "shaders/LambertShader.h"
#include "shaders/PhongShader.h"
#include "shaders/SimpleShadowShader.h"
#include "shaders/RefractionShader.h"
#include "shaders/SimpleTextureShader.h"
#include "shaders/CookTorranceShader.h"
#include "lights/PointLight.h"
#include "lights/AmbientLight.h"
#include "lights/SpotLight.h"

#include <fstream>
#include <iostream>
#include <string>
#include <exception>
#include <algorithm>
#include <cctype>

#include "third-party/json.h"
using json = nlohmann::json;

extern UBO uniformBufferData;

#define LOAD_ASSERT(cond, msg) \
    if (!(cond)) { \
        RT_ERROR("Error loading scene: {0}", msg); \
        throw std::runtime_error(msg); \
    }

static std::unordered_map<std::string, std::shared_ptr<Shader>> s_Shaders;

static Vec3 GetJsonVec3(const json& item) {
    LOAD_ASSERT(item.is_array() && item.size() == 3, "item must be an array of 3 elements");
    return glm::vec3(item[0], item[1], item[2]);
}

static float GetJsonFloat(const json& item) {
    LOAD_ASSERT(item.is_number(), "item must be a number");
    return float(item);
}

bool LoadSettings(class Scene& scene, const json& settings) {
    // Currently no settings to load
    if (settings.contains("camera")) {
        LOAD_ASSERT(settings["camera"].is_object(), "'camera' must be an object");
        if (settings["camera"].contains("position")) SetCameraPosition(GetJsonVec3(settings["camera"]["position"]));
        if (settings["camera"].contains("forward")) SetCameraForward(GetJsonVec3(settings["camera"]["forward"]));
        if (settings["camera"].contains("up")) SetCameraUp(GetJsonVec3(settings["camera"]["up"]));
        if (settings["camera"].contains("fov")) SetCameraFOV(GetJsonFloat(settings["camera"]["fov"]));
    } 
    if (settings.contains("gi")) Params::s_EnableGI = settings["gi"].get<bool>();
    if (settings.contains("env_map")) {
        uniformBufferData.u_environmentMapIndex = (new Texture(settings["env_map"]))->GetId();
    }

    return true;
}

bool LoadShader(class Scene& scene, const json& shader) {
    LOAD_ASSERT(shader.contains("type"), "Shader must have a 'type' field");
    LOAD_ASSERT(shader.contains("name"), "Shader must have a 'name' field");
    std::string type = shader["type"];
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    std::string name = shader["name"];
    std::shared_ptr<Shader> shaderPtr = nullptr;
    
    if (type == "flat" || type == "flatshader") {
        LOAD_ASSERT(shader.contains("color"), "FlatShader must have a 'color' field");
        shaderPtr = std::make_shared<FlatShader>(GetJsonVec3(shader["color"]));
    } else if (type == "refraction" || type == "refractionshader") {
        LOAD_ASSERT(shader.contains("indexInside"), "RefractionShader must have an 'indexInside' field");
        LOAD_ASSERT(shader.contains("indexOutside"), "RefractionShader must have an 'indexOutside' field");
        shaderPtr = std::make_shared<RefractionShader>(
            GetJsonFloat(shader["indexInside"]),
            GetJsonFloat(shader["indexOutside"])
        );
    } else if (type == "mirror" || type == "mirrorshader") {
        LOAD_ASSERT(shader.contains("throughput"), "MirrorShader must have a 'throughput' field");
        shaderPtr = std::make_shared<MirrorShader>(GetJsonVec3(shader["throughput"]));
    } else if (type == "lambert" || type == "lambertshader") {
        LOAD_ASSERT(shader.contains("diffuseColor"), "LambertShader must have a 'diffuseColor' field");
        shaderPtr = std::make_shared<LambertShader>(GetJsonVec3(shader["diffuseColor"]));
    } else if (type == "phong" || type == "phongshader") {
        LOAD_ASSERT(shader.contains("diffuseColor"), "PhongShader must have a 'diffuseColor' field");
        LOAD_ASSERT(shader.contains("diffuseCoefficient"), "PhongShader must have a 'diffuseCoefficient' field");
        LOAD_ASSERT(shader.contains("specularColor"), "PhongShader must have a 'specularColor' field");
        LOAD_ASSERT(shader.contains("specularCoefficient"), "PhongShader must have a 'specularCoefficient' field");
        LOAD_ASSERT(shader.contains("shininessExponent"), "PhongShader must have a 'shininessExponent' field");
        shaderPtr = std::make_shared<PhongShader>(
            GetJsonVec3(shader["diffuseColor"]),
            GetJsonFloat(shader["diffuseCoefficient"]),
            GetJsonVec3(shader["specularColor"]),
            GetJsonFloat(shader["specularCoefficient"]),
            GetJsonFloat(shader["shininessExponent"])
        );
    } else if (type == "simpleshadow" || type == "simpleshadowshader") {
        LOAD_ASSERT(shader.contains("objectColor"), "SimpleShadowShader must have an 'objectColor' field");
        shaderPtr = std::make_shared<SimpleShadowShader>(GetJsonVec3(shader["objectColor"]));
    } else if (type == "cooktorrance" || type == "cooktorranceshader") {
        LOAD_ASSERT(shader.contains("diffuseColor"), "CookTorranceShader must have a 'diffuseColor' field");
        LOAD_ASSERT(shader.contains("ctColor"), "CookTorranceShader must have a 'ctColor' field");
        LOAD_ASSERT(shader.contains("IOR"), "CookTorranceShader must have an 'IOR' field");
        LOAD_ASSERT(shader.contains("roughness"), "CookTorranceShader must have a 'roughness' field");
        float diffuseCoefficient = shader.contains("diffuseCoefficient") ? GetJsonFloat(shader["diffuseCoefficient"]) : PI / 2.0f;
        float ctCoefficient = shader.contains("ctCoefficient") ? GetJsonFloat(shader["ctCoefficient"]) : PI / 2.0f;
        shaderPtr = std::make_shared<CookTorranceShader>(
            GetJsonVec3(shader["diffuseColor"]),
            GetJsonVec3(shader["ctColor"]),
            GetJsonFloat(shader["IOR"]),
            GetJsonFloat(shader["roughness"]),
            diffuseCoefficient,
            ctCoefficient
        );
    }
    // else if (type == "simpleTexture") {
    //     LOAD_ASSERT(shader.contains("texture"), "SimpleTextureShader must have a 'texture' field");
    //     TextureID textureId = static_cast<TextureID>(shader["texture"].get<int>());
    //     if (shader.contains("color")) {
    //         shaderPtr = std::make_shared<SimpleTextureShader>(GetJsonVec3(shader["color"]), textureId);
    //     } else {
    //         shaderPtr = std::make_shared<SimpleTextureShader>(textureId);
    //     }
    // } 

    LOAD_ASSERT(shaderPtr, "Unknown shader type: " + type);
    scene.AddShader(shaderPtr);
    s_Shaders[name] = shaderPtr;
    return true;
}

bool LoadLight(class Scene& scene, const json& light) {
    LOAD_ASSERT(light.contains("type"), "Light must have a 'type' field");
    std::string type = light["type"];
    std::transform(type.begin(), type.end(), type.begin(), ::tolower);
    std::shared_ptr<Light> lightPtr = nullptr;

    if (type == "point" || type == "pointlight") {
        LOAD_ASSERT(light.contains("position"), "PointLight must have a 'position' field");
        LOAD_ASSERT(light.contains("intensity"), "PointLight must have an 'intensity' field");
        float radius = light.contains("radius") ? GetJsonFloat(light["radius"]) : 0.0f;
        Vec3 color = light.contains("color") ? GetJsonVec3(light["color"]) : VecUtils::One;
        lightPtr = std::make_shared<PointLight>(
            GetJsonVec3(light["position"]),
            GetJsonFloat(light["intensity"]),
            radius,
            color
        );
    } else if (type == "ambient" || type == "ambientlight") {
        LOAD_ASSERT(light.contains("intensity"), "AmbientLight must have an 'intensity' field");
        Vec3 color = light.contains("color") ? GetJsonVec3(light["color"]) : VecUtils::One;
        lightPtr = std::make_shared<AmbientLight>(
            GetJsonFloat(light["intensity"]),
            color
        );
    } else if (type == "spot" || type == "spotlight") {
        LOAD_ASSERT(light.contains("position"), "SpotLight must have a 'position' field");
        LOAD_ASSERT(light.contains("direction"), "SpotLight must have a 'direction' field");
        LOAD_ASSERT(light.contains("alphaMin"), "SpotLight must have an 'alphaMin' field");
        LOAD_ASSERT(light.contains("alphaMax"), "SpotLight must have an 'alphaMax' field");
        LOAD_ASSERT(light.contains("intensity"), "SpotLight must have an 'intensity' field");
        Vec3 color = light.contains("color") ? GetJsonVec3(light["color"]) : VecUtils::One;
        lightPtr = std::make_shared<SpotLight>(
            GetJsonVec3(light["position"]),
            GetJsonVec3(light["direction"]),
            GetJsonFloat(light["alphaMin"]),
            GetJsonFloat(light["alphaMax"]),
            GetJsonFloat(light["intensity"]),
            color
        );
    }

    LOAD_ASSERT(lightPtr, "Unknown light type: " + type);
    scene.AddLight(lightPtr);
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
    } else if (type == "mesh") {
        LOAD_ASSERT(primitiveData.contains("filename"), "Mesh must have a 'filename' field");
        std::string filename = std::string(primitiveData["filename"]);
        Vec3 scale = VecUtils::One;
        if (primitiveData.contains("scale")) {
            LOAD_ASSERT(primitiveData["scale"].is_array() && primitiveData["scale"].size() == 3, "Mesh scale must be Vec3");
            scale = Vec3(primitiveData["scale"][0], primitiveData["scale"][1], primitiveData["scale"][2]);
        }
        Vec3 translation = VecUtils::Zero;
        if (primitiveData.contains("translation")) {
            LOAD_ASSERT(primitiveData["translation"].is_array() && primitiveData["translation"].size() == 3, "Mesh translation must be Vec3");
            translation = Vec3(primitiveData["translation"][0], primitiveData["translation"][1], primitiveData["translation"][2]);
        }
        bool flipU = false;
        bool flipV = false;
        std::shared_ptr<Mesh> mesh = std::make_shared<Mesh>(filename.c_str(), s_Shaders[shaderName], scale, translation, flipU, flipV);
        scene.AddPrimitive(mesh);
    } else {
        RT_ERROR("Unknown primitive type: {0}", type);
        return false;
    }

    return true;
}

bool SceneLoader::LoadScene(class Scene& scene, const std::string& filename) {
    scene.ClearScene();
    s_Shaders.clear();

    std::ifstream f(filename);
    json data = json::parse(f);

    try {
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

        if (data.contains("lights")) {
            LOAD_ASSERT(data["lights"].is_array(), "'lights' must be an array");
            for (const auto& light : data["lights"]) {
                LOAD_ASSERT(LoadLight(scene, light), "Failed to load a light");
            }
        }

        if (data.contains("primitives")) {
            LOAD_ASSERT(data["primitives"].is_array(), "'primitives' must be an array");
            for (const auto& primitiveData : data["primitives"]) {
                LOAD_ASSERT(LoadPrimitive(scene, primitiveData), "Failed to load a primitive");
            }
        }
    } catch (std::runtime_error exception) {
        RT_ERROR("Error during scene loading from JSON: {}", exception.what());
    }
    
    
    return true;
}