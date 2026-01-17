#pragma once

#include <memory>
#include <string>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

class Window {
private:
    struct GLFWwindowDeleter {
        void operator()(GLFWwindow* ptr) const {
            if (ptr) glfwDestroyWindow(ptr);
        }
    };

    uint32_t m_width;
    uint32_t m_height;
    std::string m_title;

    std::unique_ptr<GLFWwindow, GLFWwindowDeleter> m_window;

    using FramebufferResizeCallback = std::function<void(uint32_t, uint32_t)>;
    FramebufferResizeCallback m_userFramebufferResizeCallback;

    static bool s_glfwInitialized;
    static void framebufferResizeCallbackImpl(GLFWwindow* window, int width, int height);
public:
    ~Window();
    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;
    Window(Window&&) = delete;
    Window& operator=(Window&&) = delete;

    Window(const std::string& title, uint32_t width, uint32_t height);

    bool shouldClose() const;
    void pollEvents() const;
    void waitUntilFocused() const;
    void waitEvents() const;

    vk::Extent2D getExtent() const;

    void getFramebufferSize(int* width, int* height) {
        glfwGetFramebufferSize(m_window.get(), width, height);
    }

    GLFWwindow* getGLFWwindow() const;

    void setFramebufferResizeCallback(FramebufferResizeCallback callback);

    // 创建 Vulkan Surface（使用 vk:: 风格）
    vk::SurfaceKHR createSurface(vk::Instance instance) const;

    // 静态方法：程序退出时调用（可选）
    static void terminateGLFW();
};
