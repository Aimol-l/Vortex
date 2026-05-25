#pragma once

#include <vulkan/vulkan.hpp>
#include <functional>

struct GLFWwindow;
struct ImGuiContext;

class ImGuiManager {
public:
    void init(class Context* context, GLFWwindow* window,
              vk::RenderPass renderPass, vk::Format imageFormat,
              uint32_t imageCount);
    void shutdown();
    void newFrame();
    void render(vk::CommandBuffer cmd);

    void setDrawCallback(std::function<void()> cb) { m_drawCallback = std::move(cb); }

    ~ImGuiManager() { shutdown(); }

private:
    class Context* m_vulkanContext = nullptr;
    vk::DescriptorPool m_descriptorPool;
    ImGuiContext* m_context = nullptr;
    std::function<void()> m_drawCallback;
};
