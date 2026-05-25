#include "Core/ImGuiManager.h"
#include "Core/Context.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <GLFW/glfw3.h>
#include <print>

void ImGuiManager::init(Context* context, GLFWwindow* window,
                        vk::RenderPass renderPass, vk::Format imageFormat,
                        uint32_t imageCount) {
    m_context = ImGui::CreateContext();
    ImGui::SetCurrentContext(m_context);

    auto& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Create descriptor pool for ImGui
    std::array poolSizes = {
        vk::DescriptorPoolSize{vk::DescriptorType::eCombinedImageSampler, 1},
    };
    vk::DescriptorPoolCreateInfo poolInfo{};
    poolInfo.flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    m_descriptorPool = context->getDevice().createDescriptorPool(poolInfo);
    m_vulkanContext = context;

    // Init Vulkan backend
    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = static_cast<VkInstance>(context->getInstance());
    initInfo.PhysicalDevice = static_cast<VkPhysicalDevice>(context->getPhysicalDevice());
    initInfo.Device = static_cast<VkDevice>(context->getDevice());
    initInfo.QueueFamily = context->getGraphicsQueueFamily();
    initInfo.Queue = static_cast<VkQueue>(context->getGraphicsQueue());
    initInfo.DescriptorPool = static_cast<VkDescriptorPool>(m_descriptorPool);
    initInfo.RenderPass = static_cast<VkRenderPass>(renderPass);
    initInfo.MinImageCount = imageCount;
    initInfo.ImageCount = imageCount;
    initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

    if (!ImGui_ImplVulkan_Init(&initInfo)) {
        throw std::runtime_error("ImGui Vulkan init failed");
    }

    // Upload fonts
    ImGui_ImplVulkan_CreateFontsTexture();

    // Init GLFW backend
    ImGui_ImplGlfw_InitForVulkan(window, true);

    std::println("ImGui initialized");
}

void ImGuiManager::shutdown() {
    if (!m_context) return;
    ImGui::SetCurrentContext(m_context);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(m_context);
    m_context = nullptr;
    if (m_vulkanContext && m_descriptorPool) {
        m_vulkanContext->getDevice().destroyDescriptorPool(m_descriptorPool);
        m_descriptorPool = nullptr;
    }
}

void ImGuiManager::newFrame() {
    ImGui::SetCurrentContext(m_context);
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    if (m_drawCallback) m_drawCallback();

    ImGui::Render();
}

void ImGuiManager::render(vk::CommandBuffer cmd) {
    ImGui::SetCurrentContext(m_context);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(cmd));
}
