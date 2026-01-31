#include "UserInterface.h"
#include "common/Params.h"
#include "scene/Scene.h"
#include "scene/Camera.h"
#include "vulkan/Renderer.h"
#include "common/Window.h"
#include "imgui.h"

static int s_SaveFrameDelay = 0;  // Countdown: 2 = just requested, 1 = render "Saving...", 0 = do save
static float s_SaveMessageTimer = 0.0f;

extern double s_DeltaTime;
extern UBO uniformBufferData;

void RenderImGuiSettings() {
    // Process deferred save request (from ImGui button)
    // Countdown allows "Saving..." to render before the blocking save
    if (s_SaveFrameDelay > 0) {
        s_SaveFrameDelay--;
        if (s_SaveFrameDelay == 0) {
            Renderer::SaveCurrentFrameToDisk(Params::GetResultImageName());
            s_SaveMessageTimer = 2.0f;
        }
    }

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(160, 280), ImGuiCond_FirstUseEver);
    ImGui::Begin("Raytracer Settings");

    // Stats display (FPS, Samples first)
    ImGui::Text("FPS: %.1f", s_DeltaTime > 0 ? 1.0 / s_DeltaTime : 0.0);
    ImGui::Text("Samples: %u", uniformBufferData.u_SampleIndex);

    ImGui::Separator();

    // Resolution (independent from window)
    static int resWidth = static_cast<int>(Params::s_Width);
    static int resHeight = static_cast<int>(Params::s_Height);
    bool resChanged = false;

    ImGui::Text("Resolution");
    ImGui::PushItemWidth(60);
    ImGui::InputInt("##Width", &resWidth, 0, 0);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        resWidth = std::clamp(resWidth, 1, 16000);
        resChanged = true;
    }
    ImGui::SameLine();
    ImGui::Text("x");
    ImGui::SameLine();
    ImGui::InputInt("##Height", &resHeight, 0, 0);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        resHeight = std::clamp(resHeight, 1, 16000);
        resChanged = true;
    }
    if (ImGui::Button("Fit to Window", ImVec2(143, 0))) {
        resWidth = Window::GetInstance()->GetWidth();
        resHeight = Window::GetInstance()->GetHeight();
        resChanged = true;
    }

    ImGui::PopItemWidth();

    if (resChanged) {
        Params::s_Width = static_cast<uint32_t>(resWidth);
        Params::s_Height = static_cast<uint32_t>(resHeight);
        Renderer::OnRenderResolutionChanged();
        uniformBufferData.u_SampleIndex = 0;
    }

    ImGui::Separator();

    // Max bounces (1-16) - title style
    ImGui::Text("Max Bounces");
    ImGui::PushItemWidth(143);
    int bounces = static_cast<int>(uniformBufferData.u_Raybounces);
    if (ImGui::SliderInt("##MaxBounces", &bounces, 1, 16)) {
        uniformBufferData.u_Raybounces = static_cast<uint32_t>(bounces);
        uniformBufferData.u_SampleIndex = 0;
    }

    ImGui::Separator();

    // Global Illumination
    bool gi = uniformBufferData.u_EnableGI != 0;
    if (ImGui::Checkbox("Enable GI", &gi)) {
        uniformBufferData.u_EnableGI = gi ? 1 : 0;
        uniformBufferData.u_SampleIndex = 0;
    }

    ImGui::Separator();

    // FOV slider (20-200 degrees) - title style
    ImGui::Text("FOV");
    float fov = GetCameraFOV();
    if (ImGui::SliderFloat("##FOV", &fov, -179.0f, 179.0f, "%1.0f")) {
        SetCameraFOV(fov);
        uniformBufferData.u_SampleIndex = 0;
    }
    ImGui::PopItemWidth();

    ImGui::Separator();

    // Save button with status indicator
    if (s_SaveFrameDelay > 0) {
        ImGui::BeginDisabled();
        ImGui::Button("Save Image");
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Saving...");
    } else {
        if (ImGui::Button("Save Image")) {
            s_SaveFrameDelay = 2;  // Wait 2 frames so "Saving..." renders first
        }

        // Show saved confirmation
        if (s_SaveMessageTimer > 0.0f) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Saved!");
            s_SaveMessageTimer -= static_cast<float>(s_DeltaTime);
        }
    }

    ImGui::End();
}
