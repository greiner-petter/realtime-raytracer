#include "common/Window.h"
#include "vulkan/VulkanAPI.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "common/Input.h"
#include "common/Log.h"
#include <GLFW/glfw3.h>

#include "vulkan/ShaderCompiler.h"

std::shared_ptr<Scene> s_Scene;
std::shared_ptr<Window> s_Window;
std::chrono::time_point<std::chrono::steady_clock> s_PreviousTime;
double s_DeltaTime = 0.0;
constexpr bool ENABLE_SHADER_HOT_RELOAD = true;

void InitWindow() {
    s_Window = Window::Create(WindowParams{ "Vulkan Raytracer", 1280, 720, false });
}

std::shared_ptr<Scene> InitVulkan() {
    ShaderCompiler::CompileAllShaders();
    return VulkanAPI::SetupVulkan();
}

void MainLoop() {
    uint32_t frameCount = 0;
    auto timer = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(Window::GetGLFWwindow())) {
        if (s_Scene->IsBufferDirty()) {
            s_Scene->UpdateGPUBuffers();
        }
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
                (std::string("Vulkan Raytracer - FPS: ") + std::to_string(frameCount)).c_str());
            frameCount = 0;
            timer = currentTime;
        }
        CameraUpdate(*s_Scene, s_DeltaTime);

        glfwPollEvents();
        glfwSwapInterval(0);
        Input::Tick();
    }
}

void Cleanup() {
    VulkanAPI::CleanUp(true);
}

int main() {
    Log::Init();
    s_PreviousTime = std::chrono::steady_clock::now();
    InitWindow();
    s_Scene = InitVulkan();
    s_Scene->spheres.push_back({ glm::vec4(0.0f, 0.0f, -9.0f, 0.33f) });
    s_Scene->spheres.push_back({ glm::vec4(2.0f, 0.0f, -3.0f, 0.2f) });
    MainLoop();
    Cleanup();

	return EXIT_SUCCESS;
}