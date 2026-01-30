#include "vulkan/Renderer.h"
#include "vulkan/ShaderCompiler.h"
#include "vulkan/Texture.h"
#include "vulkan/ImGuiLayer.h"
#include "scene/Camera.h"
#include "scene/Scene.h"
#include "scene/SceneLoader.h"
#include "primitives/Sphere.h"
#include "primitives/Mesh.h"
#include "primitives/Triangle.h"
#include "primitives/InfinitePlane.h"
#include "primitives/Box.h"
#include "shaders/FlatShader.h"
#include "common/Window.h"
#include "common/Input.h"
#include "common/Log.h"
#include "common/Params.h"
#include "common/ProgressBar.h"
#include "common/ArgParse.h"
#include "third-party/imgui/imgui.h"
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

    if (Params::GetInputSceneFilename() == "") {
        if (std::filesystem::exists("default.json")) {
            Params::s_InputScene = "default.json";
        }
    } else {
        SetCameraPosition(Vec3(0, 0, 5));
        SetCameraOrientation(VecUtils::Forward, VecUtils::Up);
        SetCameraFOV(90);
        auto shader = std::make_shared<FlatShader>(Vec3(1, 0, 0));
        s_Scene->AddShader(shader);
        s_Scene->AddPrimitive(std::make_shared<Box>(shader));
    }

    if (Params::GetInputSceneFilename() != "") {
        SceneLoader::LoadScene(*s_Scene, Params::GetInputSceneFilename());
    }
}

void InitVulkan() {
    ShaderCompiler::CompileAllShaders();
    Renderer::Init();
}

void RenderImGuiSettings() {
    ImGui::Begin("Raytracer Settings");

    // Ray bounces
    int bounces = static_cast<int>(uniformBufferData.u_Raybounces);
    if (ImGui::SliderInt("Max Bounces", &bounces, 1, 16)) {
        uniformBufferData.u_Raybounces = static_cast<uint32_t>(bounces);
        uniformBufferData.u_SampleIndex = 0;  // Reset accumulation
    }

    // Global Illumination
    bool gi = uniformBufferData.u_EnableGI != 0;
    if (ImGui::Checkbox("Enable GI", &gi)) {
        uniformBufferData.u_EnableGI = gi ? 1 : 0;
        uniformBufferData.u_SampleIndex = 0;
    }

    // Focus distance
    float focusDist = uniformBufferData.u_FocusDistance;
    if (ImGui::SliderFloat("Focus Distance", &focusDist, 0.1f, 100.0f)) {
        uniformBufferData.u_FocusDistance = focusDist;
        uniformBufferData.u_SampleIndex = 0;
    }

    ImGui::Separator();

    // Sample count target
    int samples = static_cast<int>(Params::s_Samples);
    if (ImGui::InputInt("Target Samples", &samples)) {
        Params::s_Samples = static_cast<uint32_t>(std::max(1, samples));
    }

    // Current progress
    ImGui::Text("Current Samples: %u", uniformBufferData.u_SampleIndex);
    ImGui::Text("FPS: %.1f", s_DeltaTime > 0 ? 1.0 / s_DeltaTime : 0.0);

    ImGui::Separator();

    // Reset button
    if (ImGui::Button("Reset Accumulation")) {
        uniformBufferData.u_SampleIndex = 0;
    }

    ImGui::End();
}

void MainLoop() {
    uint32_t frameCount = 0;
    auto timer = std::chrono::steady_clock::now();
    bool is_rendering = true;

    while (is_rendering) {
        // ImGui frame (only in interactive mode)
        if (Params::IsInteractiveMode()) {
            ImGuiLayer::BeginFrame();
            RenderImGuiSettings();
            ImGuiLayer::EndFrame();
        }

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
            glfwSetWindowTitle(Window::GetGLFWwindow(), (std::string("saving image...")).c_str());
            glfwPollEvents();
            glfwSwapInterval(0);
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