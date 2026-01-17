#include "Core/Command.h"
#include "Core/Context.h"
#include <print>
#include <stdexcept>
#include "Command.h"


// CommandManager implementation
CommandManager::CommandManager(Context* context, uint32_t framesInFlight, uint32_t swapchainImageCount)
    : m_context(context), m_framesInFlight(framesInFlight), m_currentFrameIndex(0) {

    std::println("CommandManager: Creating with {} frames in flight and {} swapchain images", framesInFlight, swapchainImageCount);

    m_perFrameData.resize(framesInFlight);
    m_imageAvailableSemaphores.resize(framesInFlight);
    m_renderFinishedSemaphores.resize(swapchainImageCount);

    auto device = m_context->getDevice();
    auto graphicsQueueFamily = m_context->getGraphicsQueueFamily();

    // 1. 创建每帧的命令池和fence（用于帧同步）
    for (uint32_t i = 0; i < framesInFlight; i++) {
        try {
            // 命令池（每帧独立，GRAPHICS 队列）
            vk::CommandPoolCreateInfo poolInfo{};
            poolInfo.queueFamilyIndex = graphicsQueueFamily;
            poolInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
            m_perFrameData[i].commandPool = device.createCommandPool(poolInfo);

            // 命令缓冲
            vk::CommandBufferAllocateInfo allocInfo{};
            allocInfo.commandPool = m_perFrameData[i].commandPool;
            allocInfo.level = vk::CommandBufferLevel::ePrimary;
            allocInfo.commandBufferCount = 1;
            m_perFrameData[i].primaryBuffer = device.allocateCommandBuffers(allocInfo)[0];

            // Fence（初始 signaled 状态，允许首帧立即运行）
            vk::FenceCreateInfo fenceInfo{};
            fenceInfo.flags = vk::FenceCreateFlagBits::eSignaled;
            m_perFrameData[i].inFlightFence = device.createFence(fenceInfo);

        } catch (const vk::SystemError& err) {
            throw std::runtime_error("Failed to create frame resources for frame " +
                std::to_string(i) + ": " + err.what());
        }
        vk::SemaphoreCreateInfo semaphoreInfo{};
        m_imageAvailableSemaphores[i] = device.createSemaphore(semaphoreInfo);
    }

    // 2. 为每个 swapchain 图像创建信号量（用于图像同步）
    for (uint32_t i = 0; i < swapchainImageCount; i++) {
        try {
            vk::SemaphoreCreateInfo semaphoreInfo{};
            m_renderFinishedSemaphores[i] = device.createSemaphore(semaphoreInfo);
        } catch (const vk::SystemError& err) {
            throw std::runtime_error("Failed to create semaphores for swapchain image " +
                std::to_string(i) + ": " + err.what());
        }
    }

    std::println("CommandManager: All {} frames initialized successfully", framesInFlight);
}

CommandManager::~CommandManager() {
   this->cleanup();
}

void CommandManager::cleanup(){
    auto device = m_context->getDevice();
    // 等待设备空闲，确保没有正在使用的资源
    device.waitIdle();
    // 销毁每帧资源
    for (size_t i = 0; i < m_perFrameData.size(); i++) {
        auto& frame = m_perFrameData[i];
        if (frame.inFlightFence) {
            device.destroyFence(frame.inFlightFence);
        }
        if( frame.primaryBuffer){
            device.freeCommandBuffers(frame.commandPool, frame.primaryBuffer);
        }
        if (frame.commandPool) {
            device.destroyCommandPool(frame.commandPool);
        }
    }
    m_perFrameData.clear();
    // 销毁信号量
    for (auto& sem : m_imageAvailableSemaphores) {
        if (sem) {
            device.destroySemaphore(sem);
        }
    }
    m_imageAvailableSemaphores.clear();
    for (auto& sem : m_renderFinishedSemaphores) {
        if (sem) {
            device.destroySemaphore(sem);
        }
    }
    m_renderFinishedSemaphores.clear();

}
uint32_t CommandManager::beginFrame(const vk::SwapchainKHR &swapchain){
    auto device = m_context->getDevice();
    // 等待 fence，但使用超时避免死锁
    // 如果 fence 未被信号化（例如窗口调整大小期间），返回 eTimeout
    vk::Result result = device.waitForFences(
        m_perFrameData[m_currentFrameIndex].inFlightFence,VK_TRUE,1000000000ULL
    );
    if (result == vk::Result::eTimeout) {
        return UINT32_MAX;
    }
    if (result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to wait for fence at frame " + std::to_string(m_currentFrameIndex) + "!");
    }
    result = device.acquireNextImageKHR(
        swapchain,
        UINT64_MAX,
        m_imageAvailableSemaphores[m_currentFrameIndex],
        nullptr, // 无 fence（用 semaphore 同步）
        &m_currentImageIndex
    );
    if( result != vk::Result::eSuccess) {
        throw std::runtime_error("Failed to acquire swapchain image!");
    }
    device.resetFences(m_perFrameData[m_currentFrameIndex].inFlightFence);
    return m_currentImageIndex;
}

void CommandManager::endFrame(
    vk::CommandBuffer commandBuffer,
    const vk::SwapchainKHR& swapchain)
{

    auto device = m_context->getDevice();
    auto graphicsQueue = m_context->getGraphicsQueue();
    auto presentQueue = m_context->getPresentQueue();

    // 1. 提交命令缓冲
    vk::SubmitInfo submitInfo;
    std::vector<vk::PipelineStageFlags> waitStages = {vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submitInfo.setWaitSemaphores(m_imageAvailableSemaphores[m_currentFrameIndex])
                .setWaitDstStageMask(waitStages)
                .setCommandBuffers(commandBuffer)
                .setSignalSemaphores(m_renderFinishedSemaphores[m_currentImageIndex]);

    // 提交到图形队列（使用帧索引的fence）
    graphicsQueue.submit(submitInfo, m_perFrameData[m_currentFrameIndex].inFlightFence);

    // 2. 呈现图像（使用图像索引的信号量）
    vk::PresentInfoKHR presentInfo;
    presentInfo.setWaitSemaphores(m_renderFinishedSemaphores[m_currentImageIndex])
                .setSwapchains(swapchain)
                .setImageIndices(m_currentImageIndex);
    try{
        vk::Result result =  presentQueue.presentKHR(presentInfo);
    }catch(...){
        m_currentFrameIndex = (m_currentFrameIndex + 1) % m_framesInFlight;
        throw std::runtime_error("Failed to present swapchain image");
    }
    m_currentFrameIndex = (m_currentFrameIndex + 1) % m_framesInFlight;
}

vk::CommandBuffer CommandManager::getCurrentCommandBuffer() const {
    return m_perFrameData[m_currentFrameIndex].primaryBuffer;
}

uint32_t CommandManager::getCurrentFrameIndex() const {
    return m_currentFrameIndex;
}
