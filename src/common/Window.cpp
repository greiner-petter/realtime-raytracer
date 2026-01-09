#include "Window.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include "common/Log.h"

static Window* s_Instance = nullptr;
static bool s_WindowResized = false;
static void OnWindowResized(GLFWwindow* InWindow, int InWidth, int InHeight) {
    s_WindowResized = true;
}

static void OnWindowDrop(GLFWwindow* window, int path_count, const char* paths[]) {
    if (!Window::OnDropFileCallback) {
        return;
    }

    for (int i = 0; i < path_count; i++) {
        RT_INFO("File dropped: {}", paths[i]);
        Window::OnDropFileCallback(paths[i]);
    }
}

Window::Window(const WindowParams& InParams) {
    s_Instance = this;
    if (!glfwInit()) {
        RT_ERROR("Failed to initialize GLFW");
        return;
    }
    if (!glfwVulkanSupported()) {
        RT_ERROR("GLFW: Vulkan not supported");
        return;
    }

    // Set GLFW window hints for Vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

    m_Params = InParams;
    // TODO: Window SHARED CONTEXT
    m_Window = glfwCreateWindow(m_Params.Width, m_Params.Height, m_Params.Title.c_str(), nullptr, nullptr);
    if (!m_Window) {
        RT_ERROR("Failed to create GLFW window");
        glfwTerminate();
        return;
    }
    glfwSetWindowSizeCallback(m_Window, OnWindowResized);
    glfwSetDropCallback(m_Window, OnWindowDrop);
}
Window::~Window() {
    // Destructor implementation
}

void Window::TickWindow() {

}
uint32_t Window::GetWidth() const {
    int w, h;
    glfwGetWindowSize(m_Window, &w, &h);
    return w;
}
uint32_t Window::GetHeight() const {
    int w, h;
    glfwGetWindowSize(m_Window, &w, &h);
    return h;
}

void Window::SetResizedFlag(bool InFlag) {
    s_WindowResized = InFlag;
}

bool Window::WasWindowResized() const {
    return s_WindowResized;
}

std::shared_ptr<Window> Window::Create(const WindowParams& InParams) {
    return std::shared_ptr<Window>(new Window(InParams));
}

Window* Window::GetInstance() {
    return s_Instance;
}

GLFWwindow* Window::GetGLFWwindow() {
    return Window::GetInstance()->m_Window;
}