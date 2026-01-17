#include "Core/Window.h"
#include <stdexcept>

// 静态成员初始化
bool Window::s_glfwInitialized = false;

// Static callback wrapper
void Window::framebufferResizeCallbackImpl(GLFWwindow* window, int width, int height) {
    auto* wnd = static_cast<Window*>(glfwGetWindowUserPointer(window));
    if (wnd && wnd->m_userFramebufferResizeCallback) {
        wnd->m_userFramebufferResizeCallback(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
    }
}

Window::Window(const std::string& title, uint32_t width, uint32_t height)
    : m_width(width)
    , m_height(height)
    , m_title(title) {
    // 初始化 GLFW（只初始化一次）
    if (!s_glfwInitialized) {
        if (glfwInit() != GLFW_TRUE) {
            throw std::runtime_error("Failed to initialize GLFW");
        }
        s_glfwInitialized = true;
    }
    // 配置 GLFW 为 Vulkan 模式
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    // 创建窗口
    m_window.reset(glfwCreateWindow(
        static_cast<int>(width),
        static_cast<int>(height),
        title.c_str(),
        nullptr,
        nullptr
    ));
    if (!m_window) {
        throw std::runtime_error("Failed to create GLFW window");
    }
    glfwSetWindowUserPointer(m_window.get(), this);
    glfwSetFramebufferSizeCallback(m_window.get(), framebufferResizeCallbackImpl);
}

Window::~Window() {
    m_window.reset();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(m_window.get());
}

void Window::pollEvents() const {
    glfwPollEvents();
}

void Window::waitUntilFocused() const {
    while (!glfwGetWindowAttrib(m_window.get(), GLFW_FOCUSED)) {
        glfwWaitEvents();
    }
}

void Window::waitEvents() const {
    glfwWaitEvents();
}

vk::Extent2D Window::getExtent() const {
    int width, height;
    glfwGetFramebufferSize(m_window.get(), &width, &height);

    // 等待窗口恢复可见（最小化时尺寸为 0）
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window.get(), &width, &height);
        glfwWaitEvents();
    }

    return vk::Extent2D{
        static_cast<uint32_t>(width),
        static_cast<uint32_t>(height)
    };
}

GLFWwindow* Window::getGLFWwindow() const {
    return m_window.get();
}

void Window::setFramebufferResizeCallback(FramebufferResizeCallback callback) {
    m_userFramebufferResizeCallback = std::move(callback);
}

vk::SurfaceKHR Window::createSurface(vk::Instance instance) const {
    VkSurfaceKHR surface;
    VkResult result = glfwCreateWindowSurface(
        static_cast<VkInstance>(instance),
        m_window.get(),
        nullptr,
        &surface
    );
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create window surface!");
    }
    return vk::SurfaceKHR(surface);
}

void Window::terminateGLFW() {
    if (s_glfwInitialized) {
        glfwTerminate();
        s_glfwInitialized = false;
    }
}
