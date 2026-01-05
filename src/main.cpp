#include "common/Window.h"
#include "vulkan/VulkanAPI.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "primitives/Sphere.h"
#include "primitives/Mesh.h"
#include "primitives/Triangle.h"
#include "primitives/InfinitePlane.h"
#include "common/Input.h"
#include "common/Log.h"
#include <GLFW/glfw3.h>

#include "vulkan/ShaderCompiler.h"

extern VkImage offscreenImage;

std::shared_ptr<Scene> s_Scene;
std::shared_ptr<Window> s_Window;
std::chrono::time_point<std::chrono::steady_clock> s_PreviousTime;
double s_DeltaTime = 0.0;
constexpr bool ENABLE_SHADER_HOT_RELOAD = true;
extern UBO uniformBufferData;

void InitWindow() {
    s_Window = Window::Create(WindowParams{ "Vulkan Raytracer", 1280, 720, false });
}

void InitScene() {
    s_Scene = std::make_shared<Scene>();
    s_Scene->AddPrimitive(Sphere{ glm::vec4(0.0f, 0.0f, -9.0f, 0.33f) });
    s_Scene->AddPrimitive(Sphere{ glm::vec4(2.0f, 0.0f, -3.0f, 1.2f) });
    

    auto loadedPrimitives = Mesh::LoadObj("data/teapot.obj", Vec3(1.0f), Vec3(-3.0f, 0.0f, -8.0f), false, false);
    for (const auto& prim : loadedPrimitives) {
        s_Scene->m_Primitives.push_back(prim);
    }

    s_Scene->AddPrimitive(Triangle{
        glm::vec4(-1.0f, -1.0f, -5.0f, 0.0f), 
        glm::vec4(1.0f, -1.0f, -5.0f, 0.0f), 
        glm::vec4(0.0f, 1.0f, -5.0f, 0.0f)
    });

    s_Scene->AddPrimitive(InfinitePlane{ glm::vec4(0,0,5,0), glm::vec4(0,0,-1,0)});
}

void InitVulkan() {
    ShaderCompiler::CompileAllShaders();
    VulkanAPI::SetupVulkan();
}

void MainLoop() {
    uint32_t frameCount = 0;
    auto timer = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(Window::GetGLFWwindow())) {
        s_Scene->UpdateGPUBuffers();
        VulkanAPI::Draw();
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
            VulkanAPI::SaveImageToDisk(offscreenImage, "result.png");
        }

        glfwPollEvents();
        glfwSwapInterval(0);
        Input::Tick();
    }
}

void Cleanup() {
    VulkanAPI::FullCleanUp();
}

int main() {
    Log::Init();
    s_PreviousTime = std::chrono::steady_clock::now();
    InitWindow();
    InitVulkan();
    InitScene();
    MainLoop();
    Cleanup();

	return EXIT_SUCCESS;
}