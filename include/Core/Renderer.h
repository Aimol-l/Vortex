#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <cstdint>
#include <glm/glm.hpp>
#include "Scene/Camera.h"
#include "Core/RenderPass.h"
#include "Scene/Scene.h"
#include "Core/Context.h"
#include "Core/Swapchain.h"
#include "Core/Pipeline.h"
#include "Core/Descriptor.h"
#include "Core/Command.h"
#include <vulkan/vulkan.hpp>


template<typename T>
concept ValidUBO = std::is_trivially_copyable_v<T> && requires { sizeof(T) > 0; };

class Renderer {
public:
    bool m_framebufferResized = false;
    // --- 构造与析构 ---
    explicit Renderer(std::unique_ptr<Window>& window);
    ~Renderer();
    // 禁止拷贝和移动，Vulkan 对象管理复杂，不适合浅拷贝
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    // --- 核心渲染接口 ---
    void render(const std::unique_ptr<Scene>& scene);
    void waitForIdle(); // 用于在程序退出前等待GPU完成所有工作

    // 窗口大小改变时重建swapchain
    void recreateSwapchainAndDependencies();

    Context* getContext() {
        return m_context.get();
    }
    DescriptorManager* getDescriptorManager() {
        return m_descriptorManager.get();
    }
private:
    static constexpr int MAX_FRAMES_IN_FLIGHT = 2; // 提交的最大帧数
    // --- 核心组件 ---
    std::unique_ptr<Context> m_context;
    std::unique_ptr<SwapchainManager> m_swapchain;
    std::unique_ptr<RenderPassManager> m_mainRenderPass;
    std::unique_ptr<PipelineManager> m_pipelineManager;
    std::unique_ptr<CommandManager> m_commandManager;
    std::unique_ptr<DescriptorManager> m_descriptorManager; // 负责创建和管理布局、池、集

    // --- 帧相关资源 ---
    vk::Image m_depthImage = nullptr; 
    vk::ImageView m_depthImageView = nullptr;
    vk::DeviceMemory m_depthImageMemory = nullptr; 
    std::vector<vk::Framebuffer> m_swapchainFramebuffers;

    struct FrameUBOs {
        std::vector<vk::Buffer> camera;
    } m_frameUBOs;
    struct ObjectUBOs {
        std::vector<vk::Buffer> transform;
        std::vector<vk::Buffer> light;
        std::vector<vk::Buffer> material;
    } m_objectUBOs;
    std::vector<VmaAllocation> m_frameAllocations; // 所有帧级 allocation
    std::vector<VmaAllocation> m_objectAllocations;

    void createFrameUBOs();
    void createObjectUBOs(size_t);
    void createFramebuffers();
    void createDepthResources();

    void cleanupUBOs();
    void cleanupFramebuffers();
    void cleanupDepthResources();
    
    void updateFrameUBO(uint32_t frame, const CameraUBO& cam);
    void updateObjectUBO(size_t objIdx, const TransformUBO& trans, const LightUBO& light, const MaterialUBO& mat);

    std::unique_ptr<RenderPassManager> createMainRenderPass(vk::Format color, vk::Format depth);

    // 通用创建函数
    template<ValidUBO T>
    void createUniformBuffer(vk::Buffer& buffer, VmaAllocation& alloc) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = sizeof(T);
        bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VmaAllocationCreateInfo allocInfo{};
        allocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

        VkBuffer buf;
        vmaCreateBuffer(m_context->getVmaAllocator(), &bufferInfo, &allocInfo, &buf, &alloc, nullptr);
        buffer = buf;
    }
};
