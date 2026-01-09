#include "vulkan/Renderer.h"
#include "vulkan/ShaderCompiler.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "scene/SceneLoader.h"
#include "primitives/Sphere.h"
#include "primitives/Mesh.h"
#include "primitives/Triangle.h"
#include "primitives/InfinitePlane.h"
#include "primitives/Box.h"
#include "shaders/FlatShader.h"
#include "shaders/MirrorShader.h"
#include "common/Window.h"
#include "common/Input.h"
#include "common/Log.h"
#include <GLFW/glfw3.h>


std::shared_ptr<Scene> s_Scene;
std::shared_ptr<Window> s_Window;
std::chrono::time_point<std::chrono::steady_clock> s_PreviousTime;
double s_DeltaTime = 0.0;
constexpr bool ENABLE_SHADER_HOT_RELOAD = true;
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

    std::shared_ptr<FlatShader> red = std::make_shared<FlatShader>(Vec3(1, 0, 0));
    std::shared_ptr<FlatShader> green = std::make_shared<FlatShader>(Vec3(0, 1, 0));
    std::shared_ptr<FlatShader> blue = std::make_shared<FlatShader>(Vec3(0, 0, 1));
    std::shared_ptr<MirrorShader> mirror = std::make_shared<MirrorShader>(Vec3(0.8));

    std::shared_ptr<Sphere> redSphere = std::make_shared<Sphere>(Vec3(0.0f, 0.0f, -9.0f), 0.33f, red);
    std::shared_ptr<Sphere> mirrorSphere = std::make_shared<Sphere>(Vec3(2.0f, 0.0f, -3.0f), 1.2f, mirror);
    std::shared_ptr<Triangle> greenTriangle = std::make_shared<Triangle>(Vec3(-1.0f, -1.0f, -5.0f), Vec3(1.0f, -1.0f, -5.0f), Vec3(0.0f, 1.0f, -5.0f), green);
    std::shared_ptr<InfinitePlane> mirrorPlane = std::make_shared<InfinitePlane>(Vec3(0, 0, 5), Vec3(0, 0, -1), mirror);
    std::shared_ptr<Box> redBox = std::make_shared<Box>(Vec3(0, 0, -15), Vec3(1), blue);

    s_Scene->AddShader(red);
    s_Scene->AddShader(green);
    s_Scene->AddShader(blue);
    s_Scene->AddShader(mirror);

    s_Scene->AddPrimitive(redSphere);
    s_Scene->AddPrimitive(mirrorSphere);
    s_Scene->AddPrimitive(greenTriangle);
    s_Scene->AddPrimitive(mirrorPlane);
    s_Scene->AddPrimitive(redBox);
    

    // auto loadedPrimitives = Mesh::LoadObj("data/teapot.obj", Vec3(1.0f), Vec3(-3.0f, 0.0f, -8.0f), false, false);
    // for (const auto& prim : loadedPrimitives) {
    //     s_Scene->m_Primitives.push_back(prim);
    // }
}

void InitVulkan() {
    ShaderCompiler::CompileAllShaders();
    Renderer::Init();
}

void MainLoop() {
    uint32_t frameCount = 0;
    auto timer = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(Window::GetGLFWwindow())) {
        s_Scene->UpdateGPUBuffers();
        Renderer::Draw();
        frameCount++;

        if (ENABLE_SHADER_HOT_RELOAD) {
            ShaderCompiler::CompileAllShaders();
        }

        auto currentTime = std::chrono::steady_clock::now();
		auto elapsed = currentTime.time_since_epoch() - s_PreviousTime.time_since_epoch();

		s_DeltaTime = elapsed.count() / 1000000000.0;
        s_PreviousTime = currentTime;

        //update
        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - timer).count() >= 1.0) {
            glfwSetWindowTitle(Window::GetGLFWwindow(), 
                (std::string("Vulkan Raytracer - FPS: ") + std::to_string(frameCount) + " - Samples: " + std::to_string(uniformBufferData.u_SampleIndex)).c_str());
            frameCount = 0;
            timer = currentTime;
        }
        CameraUpdate(*s_Scene, s_DeltaTime);

        if (Input::IsKeyPressed(Key::LeftControl) && Input::IsKeyPressed(Key::S)) {
            Renderer::SaveCurrentFrameToDisk("result.png");
        }

        glfwPollEvents();
        glfwSwapInterval(0);
        Input::Tick();
    }
}

void Cleanup() {
    Renderer::Cleanup();
}

int main() {
    Log::Init();
    InitWindow();
    InitVulkan();
    InitScene();
    MainLoop();
    Cleanup();

	return EXIT_SUCCESS;
}