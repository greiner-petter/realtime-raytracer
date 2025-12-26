#include "common/Window.h"
#include "common/VulkanAPI.h"
#include "camera/RT_Camera.h"
#include "common/Input.h"
#include "common/Log.h"
#include <GLFW/glfw3.h>

std::shared_ptr<Window> m_Window;
std::chrono::time_point<std::chrono::steady_clock> m_PreviousTime;
double m_DeltaTime = 0.0;

void InitWindow() {
    m_Window = Window::Create(WindowParams{ "Vulkan Raytracer", 1280, 720, false });
}

void InitVulkan() {
    VulkanAPI::SetupVulkan();
}

void MainLoop() {
    uint32_t frameCount = 0;
    auto timer = std::chrono::steady_clock::now();
    while (!glfwWindowShouldClose(Window::GetGLFWwindow())) {
        VulkanAPI::UpdateUniformData();
        VulkanAPI::UpdateSceneData();
        VulkanAPI::Draw();
        frameCount++;

        auto currentTime = std::chrono::steady_clock::now();
		auto elapsed = currentTime.time_since_epoch() - m_PreviousTime.time_since_epoch();

		m_DeltaTime = elapsed.count() / 1000000000.0;
        m_PreviousTime = currentTime;

        //update
        if (std::chrono::duration_cast<std::chrono::seconds>(currentTime - timer).count() >= 1.0) {
            glfwSetWindowTitle(Window::GetGLFWwindow(), 
                (std::string("Vulkan Raytracer - FPS: ") + std::to_string(frameCount)).c_str());
            frameCount = 0;
            timer = currentTime;
        }
        CameraUpdate();

        glfwPollEvents();
        Input::Tick();
    }
}

void Cleanup() {
    VulkanAPI::CleanUp(true);
}

int main() {
    Log::Init();
    m_PreviousTime = std::chrono::steady_clock::now();
    InitWindow();
    InitVulkan();
    MainLoop();
    Cleanup();

	return EXIT_SUCCESS;
}