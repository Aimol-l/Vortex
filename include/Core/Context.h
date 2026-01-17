#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <optional>
#include <unordered_set>
#include "Core/Window.h"
#include "3rd/vk_mem_alloc.h"  // 只包含头文件，不定义实现

struct GLFWwindow; // 前向声明

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> computeFamily;
    std::optional<uint32_t> transferFamily;
    bool isComplete() const {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};
class Context {
private:
    bool m_enableValidationLayers = true;

    vk::Device m_logDevice;                 // 创建的逻辑设备
    vk::Instance m_instance;                // 创建的vk实例
    vk::SurfaceKHR m_surface;               // glfw创建的窗口表面,调用Window类的函数获得
    vk::PhysicalDevice m_phyDevice;         // 选择的本地物理设备

    vk::Queue m_computeQueue;
    vk::Queue m_presentQueue;
    vk::Queue m_transferQueue;
    vk::Queue m_graphicsQueue;

    vk::CommandPool m_transientCommandPool;
    vk::CommandPool m_graphicsCommandPool;

    VmaAllocator m_vmaAllocator;            

    QueueFamilyIndices m_queuefamily;       // 选择的队列族索引

    VkDebugUtilsMessengerEXT m_debugMessenger;
    PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT = nullptr;

    const std::vector<const char*> m_validationLayers = {"VK_LAYER_KHRONOS_validation"};
    const std::vector<const char*> m_deviceExtensions = {"VK_KHR_swapchain"};               // 必要的扩展
private:
    void createInstance();
    void setupDebugMessenger();
    void pickPhysicalDevice();
    void createLogicalDevice();
    void createVmaAllocator();
    void createCommandPool();
    bool checkValidationLayerSupport();
public:
    ~Context(){
        // 1. 销毁 VMA 分配器
        if (m_vmaAllocator != VK_NULL_HANDLE) {
            vmaDestroyAllocator(m_vmaAllocator);
            m_vmaAllocator = VK_NULL_HANDLE;
        }

        // 2. 销毁命令池
        if (m_graphicsCommandPool) {
            m_logDevice.destroyCommandPool(m_graphicsCommandPool);
            m_graphicsCommandPool = nullptr;
        }
        if (m_transientCommandPool) {
            m_logDevice.destroyCommandPool(m_transientCommandPool);
            m_transientCommandPool = nullptr;
        }

        // 3. 销毁逻辑设备
        if (m_logDevice) {
            m_logDevice.destroy();
            m_logDevice = nullptr;
        }

        // 4. 销毁调试 messenger
        if (m_debugMessenger && pfnDestroyDebugUtilsMessengerEXT) {
            pfnDestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
            m_debugMessenger = nullptr;
        }

        // 5. 销毁 surface
        if (m_surface) {
            m_instance.destroySurfaceKHR(m_surface);
            m_surface = nullptr;
        }

        // 6. 销毁实例
        if (m_instance) {
            m_instance.destroy();
            m_instance = nullptr;
        }
    }
    Context(std::unique_ptr<Window>& window);

    // 禁止拷贝，允许移动
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    Context(Context&&) = delete;
    Context& operator=(Context&&) = delete;

    // Getters (返回 const 引用或值)
    const vk::Device& getDevice() const { return m_logDevice;}
    const vk::Instance& getInstance() const { return m_instance;}
    
    vk::SurfaceKHR getSurface() const { return m_surface;}
    vk::PhysicalDevice getPhysicalDevice() const { return m_phyDevice; }
    const VmaAllocator& getVmaAllocator() const { return m_vmaAllocator; }

    vk::Queue getComputeQueue() const { return m_computeQueue;}
    vk::Queue getPresentQueue() const { return m_presentQueue;}
    vk::Queue getTransferQueue() const { return m_transferQueue;}
    vk::Queue getGraphicsQueue() const { return m_graphicsQueue;}

    vk::CommandPool getTransientCommandPool() const { return m_transientCommandPool; }
    vk::CommandPool getGraphicsCommandPool() const { return m_graphicsCommandPool; }

    uint32_t getPresentQueueFamily() const { return m_queuefamily.presentFamily.value();}
    uint32_t getGraphicsQueueFamily() const { return m_queuefamily.graphicsFamily.value();}
    uint32_t getComputeQueueFamily() const { return m_queuefamily.computeFamily.value();}
    uint32_t getTransferQueueFamily() const { return m_queuefamily.transferFamily.value();}
};