#pragma once
#include <vector>
#include <vulkan/vulkan.hpp>
#include "Core/Context.h"
#include "Core/Window.h"

class SwapchainManager {

static const uint32_t MAX_FRAMES_IN_FLIGHT = 3;

public:
    SwapchainManager(Context* context, Window* window);
    ~SwapchainManager(){
        this->cleanup();
    }

    // 禁止拷贝和移动
    SwapchainManager(const SwapchainManager&) = delete;
    SwapchainManager& operator=(const SwapchainManager&) = delete;
    SwapchainManager(SwapchainManager&&) = delete;
    SwapchainManager& operator=(SwapchainManager&&) = delete;

    void recreate();
    bool isValid() const{return m_isValid;}


    size_t getImageCount() const;
    vk::Extent2D getExtent() const;
    vk::Format getImageFormat() const;
    const vk::SwapchainKHR& getSwapchain() const;
    const vk::ImageView& getImageView(size_t index) const;
    const std::vector<vk::ImageView>& getImageViews()const;
private:
    Context* m_context;
    Window* m_window;

    bool m_isValid = false;

    // --- 交换链核心资源 ---
    vk::SwapchainKHR m_swapchain;
    vk::Format m_swapchainImageFormat;
    vk::Extent2D m_swapchainExtent;
    std::vector<vk::Image> m_swapchainImages;
    std::vector<vk::ImageView> m_swapchainImageViews;
    
    void cleanup();
    // --- 私有辅助函数 ---
    void createImageViews();
    void createSwapchain();

    vk::Extent2D chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);
    vk::SurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
    vk::PresentModeKHR chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
};
