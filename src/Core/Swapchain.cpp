#include <print>
#include "Swapchain.h"
#include "Core/Context.h"
#include "Core/Window.h"

vk::Extent2D SwapchainManager::chooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        std::println("Using current swap extent: {}x{}", capabilities.currentExtent.width, capabilities.currentExtent.height);
        return capabilities.currentExtent;
    } else {
        int width, height;
        m_window->getFramebufferSize(&width, &height); // 假设你的Window类有这个函数
        vk::Extent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        std::println("Requested swap extent: {}x{}", actualExtent.width, actualExtent.height);
        actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
        return actualExtent;
    }
}
vk::SurfaceFormatKHR SwapchainManager::chooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats) {
    for (const auto& format : availableFormats) {
        if (format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear) {
            return format;
        }
    }
    return availableFormats[0]; // 默认
}
void SwapchainManager::cleanup() {
    if (!m_isValid) {
        return; // 如果已经无效，无需清理
    }
    auto device = m_context->getDevice();
    // 移除 waitIdle() - 调用者应该负责同步

    // 1. 清理图像视图
    for (auto imageView : m_swapchainImageViews) {
        device.destroyImageView(imageView);
    }
    m_swapchainImageViews.clear();

    // 2. 清理交换链图像容器（swapchain 销毁时会自动清理 images）
    m_swapchainImages.clear();

    // 3. 清理交换链
    if (m_swapchain) {
        device.destroySwapchainKHR(m_swapchain);
        m_swapchain = nullptr;
    }

    m_isValid = false; // 标记为无效
}
// 辅助函数：创建 ImageView
void SwapchainManager::createImageViews() {
    auto device = m_context->getDevice();

    // 1. 清理旧的图像视图
    for (auto& view : m_swapchainImageViews) {
        if (view) {
            device.destroyImageView(view);
        }
    }
    m_swapchainImageViews.clear();

    // 2. 创建新的图像视图
    m_swapchainImageViews.resize(m_swapchainImages.size());

    for (size_t i = 0; i < m_swapchainImages.size(); ++i) {
        vk::ImageViewCreateInfo createInfo{};
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = vk::ImageViewType::e2D;
        createInfo.format = m_swapchainImageFormat;
        createInfo.components.r = vk::ComponentSwizzle::eIdentity;
        createInfo.components.g = vk::ComponentSwizzle::eIdentity;
        createInfo.components.b = vk::ComponentSwizzle::eIdentity;
        createInfo.components.a = vk::ComponentSwizzle::eIdentity;
        createInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;

        try {
            m_swapchainImageViews[i] = device.createImageView(createInfo);
        } catch (const vk::SystemError& err) {
            throw std::runtime_error("Failed to create image view " + std::to_string(i) + ": " + std::string(err.what()));
        }
    }

    std::println("Created {} image views for swapchain", m_swapchainImageViews.size());
}
void SwapchainManager::recreate() {
    this->cleanup();
    this->createSwapchain();
    this->createImageViews();
}

vk::PresentModeKHR SwapchainManager::chooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes) {
    for (const auto& mode : availablePresentModes) {
        if (mode == vk::PresentModeKHR::eMailbox) {
            return mode;
        }
    }
    return vk::PresentModeKHR::eFifo;
}
void SwapchainManager::createSwapchain() {
    auto device = m_context->getDevice();
    auto surface = m_context->getSurface();
    auto physicalDevice = m_context->getPhysicalDevice();

    // 1. 查询交换链的详细信息
    vk::SurfaceCapabilitiesKHR capabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> formats = physicalDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> presentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    // 2. 选择最佳的交换链属性
    vk::SurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(formats);
    vk::PresentModeKHR presentMode = chooseSwapPresentMode(presentModes);
    vk::Extent2D extent = this->chooseSwapExtent(capabilities);

    uint32_t imageCount = 0;
    if (presentMode == vk::PresentModeKHR::eMailbox) {
        imageCount = 3;  // MAILBOX 需要至少3个避免阻塞
    } else {
        imageCount = 2;  // 其他模式双缓冲足够
    }
    imageCount = std::max(imageCount, capabilities.minImageCount);
    if (capabilities.maxImageCount > 0) {
        imageCount = std::min(imageCount, capabilities.maxImageCount);
    }

    // 4. 填充创建信息结构体
    vk::SwapchainCreateInfoKHR createInfo{};
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment 
                            | vk::ImageUsageFlagBits::eTransferSrc
                            | vk::ImageUsageFlagBits::eTransferDst;

    auto graphicsQueueFamily = m_context->getGraphicsQueueFamily();
    auto presentQueueFamily = m_context->getPresentQueueFamily();

    // 5. 设置图像共享模式
    if (graphicsQueueFamily != presentQueueFamily) {
        std::array<uint32_t, 2> queueFamilyIndices{graphicsQueueFamily, presentQueueFamily};
        createInfo.imageSharingMode = vk::SharingMode::eConcurrent;
        createInfo.queueFamilyIndexCount = static_cast<uint32_t>(queueFamilyIndices.size());
        createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
    } else {
        createInfo.imageSharingMode = vk::SharingMode::eExclusive;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque; // 不与其它窗口混合
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE; // 不渲染被遮挡的像素，提高性能

    // 保存旧的 swapchain 用于平滑过渡
    vk::SwapchainKHR oldSwapchain = m_swapchain;
    createInfo.oldSwapchain = oldSwapchain;

    // 6. 创建新的交换链
    try {
        m_swapchain = device.createSwapchainKHR(createInfo);
        std::println("Swapchain created successfully with {} images", imageCount);
    } catch (vk::SystemError& err) {
        throw std::runtime_error("failed to create swap chain! " + std::string(err.what()));
    }

    // 7. 销毁旧的 swapchain（如果有）
    if (oldSwapchain) {
        // 不需要 waitIdle()，使用 oldSwapchain 参数已经处理了同步
        device.destroySwapchainKHR(oldSwapchain);
        std::println("Old swapchain destroyed");
    }

    // 8. 获取交换链图像句柄并保存格式和尺寸
    m_swapchainImages = device.getSwapchainImagesKHR(m_swapchain);
    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;
    m_isValid = true; // 标记为有效
}
SwapchainManager::SwapchainManager(Context *context, Window *window)
    : m_context(context), m_window(window), m_isValid(false) {
    // 1. 创建交换链
    this->createSwapchain();
    // 2. 创建图像视图
    this->createImageViews();
}

const vk::SwapchainKHR& SwapchainManager::getSwapchain() const {
    return m_swapchain;
}
vk::Format SwapchainManager::getImageFormat() const {
    return m_swapchainImageFormat;
}
vk::Extent2D SwapchainManager::getExtent() const {
    return m_swapchainExtent;
}
size_t SwapchainManager::getImageCount() const {
    return m_swapchainImages.size();
}
const vk::ImageView& SwapchainManager::getImageView(size_t index) const{
    return m_swapchainImageViews.at(index);
}

const std::vector<vk::ImageView>& SwapchainManager::getImageViews()const{
    return this->m_swapchainImageViews;
}