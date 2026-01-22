#include "vulkan/Renderer.h"
#include "vulkan/ShaderCompiler.h"
#include "vulkan/Texture.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "scene/SceneLoader.h"
#include "primitives/Sphere.h"
#include "primitives/Mesh.h"
#include "primitives/Triangle.h"
#include "primitives/InfinitePlane.h"
#include "primitives/Box.h"
#include "shaders/FlatShader.h"
#include "shaders/RefractionShader.h"
#include "shaders/MirrorShader.h"
#include "shaders/SimpleShadowShader.h"
#include "shaders/SimpleTextureShader.h"
#include "shaders/LambertShader.h"
#include "lights/PointLight.h"
#include "lights/AmbientLight.h"
#include "lights/SpotLight.h"
#include "common/Window.h"
#include "common/Input.h"
#include "common/Log.h"
#include "common/Params.h"
#include "common/ProgressBar.h"
#include "common/ArgParse.h"
#include <GLFW/glfw3.h>

std::shared_ptr<Texture> s_SkyboxTexture;
std::shared_ptr<Scene> s_Scene;
std::shared_ptr<Window> s_Window;
std::chrono::time_point<std::chrono::steady_clock> s_PreviousTime;
double s_DeltaTime = 0.0;
extern UBO uniformBufferData;

static bool ends_with(std::string_view str, std::string_view suffix) {
    return str.size() >= suffix.size() && str.compare(str.size()-suffix.size(), suffix.size(), suffix) == 0;
}

void InitWindow() {
    s_PreviousTime = std::chrono::steady_clock::now();
    s_Window = Window::Create(WindowParams{ "Vulkan Raytracer", 1280, 720, false });
    Window::OnDropFileCallback = [](const std::string& filepath) {
        if (ends_with(filepath, ".json")) {
            RT_INFO("Loading scene from file: {}", filepath);
            SceneLoader::LoadScene(*s_Scene, filepath);
        }
    };
}

void InitScene() {
    s_Scene = std::make_shared<Scene>();

    // add some lights
    s_Scene->AddLight(std::make_shared<PointLight>(Vec3(0.0f, 4.0f, 0.0f), 10.0f));
    // s_Scene->AddLight(std::make_shared<AmbientLight>(0.15f));
    // s_Scene->AddLight(std::make_shared<SpotLight>(Vec3(1.0f, 0.0f, -4.0f), Vec3(0.0f, 0.0f, 1.0f), 50.0f, 70.0f, 20.0f));

    // Add shaders for the objects
    auto red = std::make_shared<LambertShader>(Vec3(1.0f, 0.3f, 0.2f));
    auto white = std::make_shared<LambertShader>(Vec3(1.0f, 1.0f, 1.0f));
    auto blue = std::make_shared<LambertShader>(Vec3(0.2f, 0.3f, 1.0f));
    auto orange = std::make_shared<LambertShader>(Vec3(1.0f, 0.5f, 0.0f));
    auto texture = std::make_shared<SimpleTextureShader>((new Texture("data/space.png"))->GetId());

    s_Scene->AddShader(red);
    s_Scene->AddShader(white);
    s_Scene->AddShader(blue);
    s_Scene->AddShader(orange);
    s_Scene->AddShader(texture);

    // Add objects
    s_Scene->AddPrimitive(std::make_shared<InfinitePlane>(Vec3(0.0f, 0.0f, +5.0f), Vec3(0.0f, 0.0f, -1.0f), white));
    s_Scene->AddPrimitive(std::make_shared<InfinitePlane>(Vec3(0.0f, 0.0f, -5.0f), Vec3(0.0f, 0.0f, +1.0f), white));
    s_Scene->AddPrimitive(std::make_shared<InfinitePlane>(Vec3(0.0f, +5.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f), white));
    s_Scene->AddPrimitive(std::make_shared<InfinitePlane>(Vec3(0.0f, -5.0f, 0.0f), Vec3(0.0f, +1.0f, 0.0f), white));
    s_Scene->AddPrimitive(std::make_shared<InfinitePlane>(Vec3(+5.0f, 0.0f, 0.0f), Vec3(-1.0f, 0.0f, 0.0f), blue));
    s_Scene->AddPrimitive(std::make_shared<InfinitePlane>(Vec3(-5.0f, 0.0f, 0.0f), Vec3(+1.0f, 0.0f, 0.0f), red));

    s_Scene->AddPrimitive(std::make_shared<Box>(Vec3(2.5f, -3.0f, 1.0f), Vec3(3.0f, 4.0f, 3.0f), red));
    s_Scene->AddPrimitive(std::make_shared<Box>(Vec3(-3.0f, -2.0f, 0.0f), Vec3(1.0f, 6.0f, 1.0f), blue));
    s_Scene->AddPrimitive(std::make_shared<Box>(Vec3(-0.5f, -4.0f, -2.0f), Vec3(2.0f, 2.0f, 2.0f), orange));
    // auto loadedPrimitives = Mesh::LoadObj("data/teapot.obj", red, Vec3(1.0f), Vec3(-3.0f, 0.0f, -8.0f), false, false);
    // for (const auto& prim : loadedPrimitives) {
    //     s_Scene->AddPrimitive(prim);
    // }

    if (Params::GetInputSceneFilename() != "") {
        SceneLoader::LoadScene(*s_Scene, Params::GetInputSceneFilename());
    }
}

void InitVulkan() {
    ShaderCompiler::CompileAllShaders();
    Renderer::Init();
}

void MainLoop() {
    uint32_t frameCount = 0;
    auto timer = std::chrono::steady_clock::now();
    bool is_rendering = true;

    while (is_rendering) {
        s_Scene->UpdateGPUBuffers();
        Renderer::Draw();
        frameCount++;

        if (Params::ENABLE_SHADER_HOT_RELOAD) {
            ShaderCompiler::CompileAllShaders();
        }

        auto currentTime = std::chrono::steady_clock::now();
		auto elapsed = currentTime.time_since_epoch() - s_PreviousTime.time_since_epoch();

		s_DeltaTime = elapsed.count() / 1000000000.0;
        s_PreviousTime = currentTime;

        //update
        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - timer).count() >= 1.0) {
            if (Params::IsInteractiveMode()) {
                Window::UpdateTitleInfo(frameCount, uniformBufferData.u_SampleIndex);
            } else {
                ProgressBar::Update(frameCount, uniformBufferData.u_SampleIndex);
            }
            frameCount = 0;
            timer = currentTime;
        }
        CameraUpdate(*s_Scene, s_DeltaTime);

        if (Input::IsKeyPressed(Key::LeftControl) && Input::IsKeyPressed(Key::S)) {
            Renderer::SaveCurrentFrameToDisk(Params::GetResultImageName());
        }

        if (Params::IsInteractiveMode()) {
            is_rendering = !glfwWindowShouldClose(Window::GetGLFWwindow());
            glfwPollEvents();
            glfwSwapInterval(0);
            Input::Tick();
        } else {
            is_rendering = uniformBufferData.u_SampleIndex < Params::GetSampleCount();
        }
    }

    if (!Params::IsInteractiveMode()) {
        ProgressBar::Update(frameCount, uniformBufferData.u_SampleIndex);
        Renderer::SaveCurrentFrameToDisk(Params::GetResultImageName());
    }
}

void Cleanup() {
    Renderer::Cleanup();
}

int main(int argc, char** argv) {
    Log::Init();
    ArgParse::ParseInput(argc, argv);
    if (Params::IsInteractiveMode()) {
        InitWindow();
    }
    InitVulkan();
    InitScene();
    MainLoop();
    Cleanup();

	return EXIT_SUCCESS;
}