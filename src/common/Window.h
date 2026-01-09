#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <functional>

struct WindowParams {
    std::string Title;
    uint32_t Width;
    uint32_t Height;
    bool Fullscreen = false;
};

class Window {
private:
    Window(const WindowParams& InParams);
public:
    virtual ~Window();

    void TickWindow();

    uint32_t GetWidth() const;
    uint32_t GetHeight() const;
    void SetResizedFlag(bool InFlag);
    bool WasWindowResized() const;

    static std::shared_ptr<Window> Create(const WindowParams& InParams);
    static Window* GetInstance();
    static struct GLFWwindow* GetGLFWwindow();

    inline static std::function<void(const std::string&)> OnDropFileCallback = nullptr;

private:
    WindowParams m_Params;
    struct GLFWwindow* m_Window = nullptr;
};