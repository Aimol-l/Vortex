#pragma once

#include <vector>
#include <stdexcept>
#include <string>
#include <vulkan/vulkan.hpp>

#include <unordered_map>
#include "Core/Context.h"

class CommandManager {
private:
    struct FrameData {
        vk::Fence inFlightFence;               // GPU 完成信号
        vk::CommandPool commandPool;           // 每帧独立命令池（避免重置开销）
        vk::CommandBuffer primaryBuffer;        // 主命令缓冲（每帧一个）
    };

    Context* m_context;
    uint32_t m_framesInFlight;
    uint32_t m_currentFrameIndex = 0;
    uint32_t m_currentImageIndex = 0;

    // CPU–GPU 同步：防止 CPU 提交过多帧，导致 GPU 积压。
    std::vector<FrameData> m_perFrameData; 
    // GPU 内部同步：通知图形队列 “swapchain image 已准备好，可以渲染了
    std::vector<vk::Semaphore> m_imageAvailableSemaphores;
    // GPU 内部同步：通知 present 队列 “渲染已完成，可以显示了”。
    std::vector<vk::Semaphore> m_renderFinishedSemaphores;

public:
    explicit CommandManager(Context* context, uint32_t framesInFlight, uint32_t swapchainImageCount);
    ~CommandManager();

    void cleanup();

    // 禁止拷贝和移动
    CommandManager(const CommandManager&) = delete;
    CommandManager& operator=(const CommandManager&) = delete;
    CommandManager(CommandManager&&) = delete;
    CommandManager& operator=(CommandManager&&) = delete;

    uint32_t beginFrame(const vk::SwapchainKHR& swapchain);
    void endFrame(vk::CommandBuffer commandBuffer, const vk::SwapchainKHR& swapchain);
    vk::CommandBuffer getCurrentCommandBuffer() const;
    uint32_t getCurrentFrameIndex() const;
};
