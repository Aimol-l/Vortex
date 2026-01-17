#include "Core/Context.h"
#include "Core/Window.h"
#include "Context.h"
#include <iostream>
#include <print>
#include <set>
#include <GLFW/glfw3.h>

bool Context::checkValidationLayerSupport() {
    auto availableLayers = vk::enumerateInstanceLayerProperties();
    for (const char* layerName : m_validationLayers) {
        std::string name(layerName);
        bool layerFound = false;
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}
void Context::createInstance(){
    //1.检查是否开启Validation
    if(m_enableValidationLayers && !checkValidationLayerSupport()){
        throw std::runtime_error("validation layers requested, but not available!");
    }
    //2.填写信息
    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName("Vortex")
        .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
        .setPEngineName("NoEngine")
        .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
        .setApiVersion(VK_API_VERSION_1_4);
    //3.获取GLFW所需的实例扩展
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    if (!glfwExtensions) {
        throw std::runtime_error("failed to get GLFW required instance extensions!");
    }

    std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

    // 如果启用验证层，添加调试扩展
    if (m_enableValidationLayers) {
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    //4.设置需要使用的层和扩展
    vk::InstanceCreateInfo createInfo;
    createInfo.setPApplicationInfo(&appInfo);
    createInfo.setPEnabledLayerNames(m_validationLayers);
    createInfo.setPEnabledExtensionNames(extensions);

    // 5.创建实例
    try {
        m_instance = vk::createInstance(createInfo);
        std::println("Vulkan instance created successfully!");
    } catch (const vk::SystemError& e) {
        throw std::runtime_error("failed to create Vulkan instance: " + std::string(e.what()));
    }
}

void Context::setupDebugMessenger(){
    if (!m_enableValidationLayers) return;
    static PFN_vkDebugUtilsMessengerCallbackEXT DebugUtilsMessengerCallback = [](
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageTypes,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData)->VkBool32 {
            std::println("{}", pCallbackData->pMessage);
            return VK_FALSE;
    };
    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = DebugUtilsMessengerCallback
    };
    pfnDestroyDebugUtilsMessengerEXT = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(static_cast<VkInstance>(m_instance), "vkDestroyDebugUtilsMessengerEXT");
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessenger = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(this->m_instance, "vkCreateDebugUtilsMessengerEXT"));
    if (vkCreateDebugUtilsMessenger) {
        VkResult result = vkCreateDebugUtilsMessenger(this->m_instance, &debugUtilsMessengerCreateInfo, nullptr,&m_debugMessenger);
        if (result)
            std::println("[VulkanBase] ERROR Failed to create a debug messenger! Error code: {}", int32_t(result));
    }
}

void Context::pickPhysicalDevice() {
    // 1. 获取所有可用的物理设备
    auto devices = m_instance.enumeratePhysicalDevices();
    if (devices.empty()) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::println("Found {} Vulkan capable device(s):", devices.size());
    // 2. 打印每个设备的信息
    for (const auto& device : devices) {
        auto properties = device.getProperties();
        std::string deviceName(properties.deviceName);
        std::string typeStr;
        switch (properties.deviceType) {
            case vk::PhysicalDeviceType::eOther:          typeStr = "Other"; break;
            case vk::PhysicalDeviceType::eIntegratedGpu:  typeStr = "Integrated GPU"; break;
            case vk::PhysicalDeviceType::eDiscreteGpu:    typeStr = "Discrete GPU"; break;
            case vk::PhysicalDeviceType::eVirtualGpu:     typeStr = "Virtual GPU"; break;
            case vk::PhysicalDeviceType::eCpu:            typeStr = "CPU"; break;
            default:                                      typeStr = "Unknown"; break;
        }
        std::println("  - Device: {}\n    Type: {}\n    API Version: {}.{}.{}\n    Driver Version: {}.{}.{}",
                     deviceName, typeStr,
                     VK_API_VERSION_MAJOR(properties.apiVersion), VK_API_VERSION_MINOR(properties.apiVersion), VK_API_VERSION_PATCH(properties.apiVersion),
                     VK_API_VERSION_MAJOR(properties.driverVersion), VK_API_VERSION_MINOR(properties.driverVersion), VK_API_VERSION_PATCH(properties.driverVersion));
    }
    // 3. 遍历设备，找到并选择最合适的那个
    vk::PhysicalDevice chosenDevice;
    QueueFamilyIndices bestIndices;
    int bestScore = -1; // 使用分数系统，-1 代表不可用或不满足条件
    for (const auto& device : devices) {
        // --- 为当前设备寻找队列族并打分 ---
        
        int currentScore = 0;
        QueueFamilyIndices currentIndices;
        const auto queueFamilies = device.getQueueFamilyProperties();
        bool foundCombinedGraphicsPresent = false;
        // 遍历当前设备的所有队列族，寻找最佳组合
        for (uint32_t i = 0; i < queueFamilies.size(); ++i) {
            const auto& queueFamily = queueFamilies[i];
            // 检查 Graphics 队列
            if (!currentIndices.graphicsFamily.has_value() && (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
                currentIndices.graphicsFamily = i;
            }
            // 检查 Present 支持 (需要 surface)
            if (!currentIndices.presentFamily.has_value() && device.getSurfaceSupportKHR(i, m_surface)) {
                currentIndices.presentFamily = i;
            }
            // 检查 Compute 队列
            if (!currentIndices.computeFamily.has_value() && (queueFamily.queueFlags & vk::QueueFlagBits::eCompute)) {
                currentIndices.computeFamily = i;
            }
            
            // 检查 Transfer 队列
            // 优先寻找专用的传输队列 (不包含Graphics能力)
            if (!currentIndices.transferFamily.has_value() && (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer) && !(queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)) {
                currentIndices.transferFamily = i;
            } else if (!currentIndices.transferFamily.has_value() && (queueFamily.queueFlags & vk::QueueFlagBits::eTransfer)) {
                 // 如果没找到专用的，再考虑兼容的
                currentIndices.transferFamily = i;
            }
            // *** 核心优化：如果 Graphics 和 Present 是同一个队列族，给予巨大加分 ***
            if (currentIndices.graphicsFamily.has_value() && currentIndices.presentFamily.has_value() &&
                currentIndices.graphicsFamily.value() == currentIndices.presentFamily.value()) {
                foundCombinedGraphicsPresent = true;
                // 对于这个设备，我们已经找到了最优的图形/呈现组合，无需再为这两个功能寻找更好的队列族
                // 但我们可以继续检查是否有更好的 Compute/Transfer 队列
            }
        }
        
        // 如果这个设备连最基本的图形和呈现都无法满足，则跳过
        if (!currentIndices.isComplete()) {
            continue;
        }
        // --- 评分系统 ---
        // 1. 队列族组合评分
        if (foundCombinedGraphicsPresent) {
            currentScore += 10000; // 巨大加分，优先选择
        }
        // 2. 设备类型评分
        const auto properties = device.getProperties();
        switch (properties.deviceType) {
            case vk::PhysicalDeviceType::eDiscreteGpu:
                currentScore += 1000; break;
            case vk::PhysicalDeviceType::eIntegratedGpu:
                currentScore += 500;  break;
            case vk::PhysicalDeviceType::eVirtualGpu:
                currentScore += 100;  break;
            default:
                // CPU 或其他类型得分很低
                break;
        }
        // 3. 其他评分标准可以在这里添加 (例如：检查是否支持所需扩展、显存大小等)
        // if (hasRequiredExtensions(device)) {
        //     currentScore += 500;
        // }
        // 比较并更新最佳选择
        if (currentScore > bestScore) {
            bestScore = currentScore;
            chosenDevice = device;
            bestIndices = currentIndices;
        }
    }
    // 4. 记录最终选择的设备
    if (chosenDevice) {
        m_phyDevice = chosenDevice;
        m_queuefamily = bestIndices; // 保存找到的最佳队列族索引组合
        
        const auto selectedProps = m_phyDevice.getProperties();
        std::println("\nSelected Device: {}", std::string(selectedProps.deviceName));
        std::println("  Graphics Queue Family: {}", m_queuefamily.graphicsFamily.value());
        std::println("  Present Queue Family:  {}", m_queuefamily.presentFamily.value());
        if (m_queuefamily.computeFamily.has_value()) {
            std::println("  Compute Queue Family:  {}", m_queuefamily.computeFamily.value());
        }
        if (m_queuefamily.transferFamily.has_value()) {
            std::println("  Transfer Queue Family: {}", m_queuefamily.transferFamily.value());
        }
        if(m_queuefamily.graphicsFamily.value() == m_queuefamily.presentFamily.value()){
             std::println("  -> Graphics and Present queues are in the same family for optimal performance.");
        }
    } else {
        throw std::runtime_error("failed to find a suitable GPU with graphics and present support!");
    }
}

void Context::createLogicalDevice() {
    // 1. 准备队列创建信息
    // 我们需要一个唯一的队列族索引集合，避免为同一个队列族重复创建信息
    std::set<uint32_t> uniqueQueueFamilies = {
        m_queuefamily.graphicsFamily.value(),
        m_queuefamily.presentFamily.value()
    };
    // 检查 compute 和 transfer 队列族是否也是唯一的
    if (m_queuefamily.computeFamily.has_value()) {
        uniqueQueueFamilies.insert(m_queuefamily.computeFamily.value());
    }
    if (m_queuefamily.transferFamily.has_value()) {
        uniqueQueueFamilies.insert(m_queuefamily.transferFamily.value());
    }
    // 为每个唯一的队列族创建一个 VkDeviceQueueCreateInfo
    float queuePriority = 1.0f; // 队列优先级，[0.0, 1.0]，必须设置
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
    for (uint32_t queueFamilyIndex : uniqueQueueFamilies) {
        vk::DeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.setQueueFamilyIndex(queueFamilyIndex);
        queueCreateInfo.setQueueCount(1); // 我们目前每个队列族只创建一个队列
        queueCreateInfo.setPQueuePriorities(&queuePriority);
        queueCreateInfos.push_back(queueCreateInfo);
    }
    // 2. 启用设备特性
    vk::PhysicalDeviceFeatures deviceFeatures = m_phyDevice.getFeatures();
    
    vk::PhysicalDeviceFeatures enabledFeatures{};
    enabledFeatures.samplerAnisotropy = VK_TRUE;

    // 4. 创建逻辑设备
    vk::DeviceCreateInfo createInfo{};
    createInfo.setQueueCreateInfoCount(static_cast<uint32_t>(queueCreateInfos.size()));
    createInfo.setPQueueCreateInfos(queueCreateInfos.data());
    
    // 设置设备特性和扩展
    createInfo.setPEnabledFeatures(&enabledFeatures);
    createInfo.setEnabledExtensionCount(static_cast<uint32_t>(m_deviceExtensions.size()));
    createInfo.setPpEnabledExtensionNames(m_deviceExtensions.data());

    try {
        m_logDevice = m_phyDevice.createDevice(createInfo);
        std::println("Logical device created successfully!");
    } catch (vk::SystemError& err) {
        throw std::runtime_error("failed to create logical device! " + std::string(err.what()));
    }
    // 5. 获取队列句柄
    m_graphicsQueue = m_logDevice.getQueue(m_queuefamily.graphicsFamily.value(), 0);
    m_presentQueue = m_logDevice.getQueue(m_queuefamily.presentFamily.value(), 0);
    if (m_queuefamily.computeFamily.has_value()) {
        m_computeQueue = m_logDevice.getQueue(m_queuefamily.computeFamily.value(), 0);
        std::println("Compute queue obtained.");
    }
    if (m_queuefamily.transferFamily.has_value()) {
        m_transferQueue = m_logDevice.getQueue(m_queuefamily.transferFamily.value(), 0);
        std::println("Transfer queue obtained.");
    }
    std::println("Queue handles obtained from the logical device.");
}
void Context::createVmaAllocator() {
    // VMA 需要知道 Vulkan 的实例、设备、物理设备和 API 版本
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = static_cast<VkPhysicalDevice>(m_phyDevice);
    allocatorInfo.device = static_cast<VkDevice>(m_logDevice);
    allocatorInfo.instance = static_cast<VkInstance>(m_instance);
    
    // VMA 3.0+ 版本推荐这样做，以获取 Vulkan 函数指针
    VmaVulkanFunctions vulkanFunctions = {};
    vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
    vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
    allocatorInfo.pVulkanFunctions = &vulkanFunctions;
    // VMA 3.0+ 推荐的 flags
    allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
    VkResult result = vmaCreateAllocator(&allocatorInfo, &m_vmaAllocator);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create VMA allocator!");
    }
}
void Context::createCommandPool() {
    // 1. 创建图形命令池
    vk::CommandPoolCreateInfo poolInfo{};
    poolInfo.setQueueFamilyIndex(m_queuefamily.graphicsFamily.value());
    poolInfo.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer); // 允许单个命令缓冲区重置
    try {
        m_graphicsCommandPool = m_logDevice.createCommandPool(poolInfo);
        std::println("Graphics command pool created successfully.");
    } catch (const vk::SystemError& err) {
        throw std::runtime_error("failed to create graphics command pool! " + std::string(err.what()));
    }

    // 2. 创建临时命令池（用于短期操作如资源传输）
    // 如果没有专用的 transfer 队列，使用 graphics 队列
    uint32_t transientQueueFamily = m_queuefamily.transferFamily.has_value()
        ? m_queuefamily.transferFamily.value()
        : m_queuefamily.graphicsFamily.value();

    poolInfo.setQueueFamilyIndex(transientQueueFamily);
    poolInfo.setFlags(vk::CommandPoolCreateFlagBits::eTransient); // 短期使用的命令缓冲区
    try {
        m_transientCommandPool = m_logDevice.createCommandPool(poolInfo);
        if (m_queuefamily.transferFamily.has_value()) {
            std::println("Transient command pool created successfully (using dedicated transfer queue).");
        } else {
            std::println("Transient command pool created successfully (using graphics queue as fallback).");
        }
    } catch (const vk::SystemError& err) {
        throw std::runtime_error("failed to create transient command pool! " + std::string(err.what()));
    }
}
Context::Context(std::unique_ptr<Window> &window){
    // 1.创建实例
    this->createInstance();
    // 2.给验证层和扩展赋值
    this->setupDebugMessenger();
    // 3.创建窗口表面（由Window类创建）
    this->m_surface = window->createSurface(this->m_instance);
    // 4.选择一个物理设备
    this->pickPhysicalDevice();
    // 5.利用选择的物理设备创建逻辑设备
    this->createLogicalDevice();
    // 6.创建分配器
    this->createVmaAllocator(); 
    // 7.创建命令池
    this->createCommandPool();
}
