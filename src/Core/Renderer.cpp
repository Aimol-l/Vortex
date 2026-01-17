#include "Core/Renderer.h"

Renderer::Renderer(std::unique_ptr<Window>& window) {
    // 1. 创建上下文 (实例、设备等)
    this->m_context = std::make_unique<Context>(window);

    // 2. 创建交换链
    this->m_swapchain = std::make_unique<SwapchainManager>(m_context.get(), window.get());

    // 3. 创建渲染通道
    this->m_mainRenderPass = this->createMainRenderPass(m_swapchain->getImageFormat(),vk::Format::eD32Sfloat);

    // 4. 创建深度资源
    this->createDepthResources();

    // 5. 创建帧缓冲
    this->createFramebuffers();

    // 6. 创建描述符管理器
    const int objectCount = 10;
    this->m_descriptorManager = std::make_unique<DescriptorManager>(m_context.get());

    // Set 0: CameraUBO (1 个 binding)
    this->m_descriptorManager->createLayout<CameraUBO>(0);

    // Set 1: TransformUBO, LightUBO, MaterialUBO, 4 个纹理采样器 (7 个 bindings)
    this->m_descriptorManager->createLayout<TransformUBO, LightUBO, MaterialUBO, TextureSampler, TextureSampler, TextureSampler, TextureSampler>(1);
    std::unordered_map<uint32_t, uint32_t> capacities = {
        {0, MAX_FRAMES_IN_FLIGHT},
        {1, static_cast<uint32_t>(objectCount)}
    };
    this->m_descriptorManager->createPool(capacities);
    this->m_descriptorManager->allocateAllSets(capacities);

    // 7. 创建帧级别的UBO
    this->createFrameUBOs();

    // 8. 创建对象级别的UBO
    this->createObjectUBOs(objectCount);

    // 将buffer和set绑定
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        m_descriptorManager->bindBufferToSet(0, i, 0, m_frameUBOs.camera[i], sizeof(CameraUBO));
    }
    for (int i = 0; i < objectCount; i++) {
        m_descriptorManager->bindBufferToSet(1, i, 0, m_objectUBOs.transform[i], sizeof(TransformUBO));
        m_descriptorManager->bindBufferToSet(1, i, 1, m_objectUBOs.light[i], sizeof(LightUBO));
        m_descriptorManager->bindBufferToSet(1, i, 2, m_objectUBOs.material[i], sizeof(MaterialUBO));
    }

    // 9. 创建渲染管线(需要使用渲染通道和描述符布局)
    this->m_pipelineManager = std::make_unique<PipelineManager>(m_context.get());
    this->m_pipelineManager->createGraphicsPipeline(
        PipelineType::Main,
        {"shaders/pbr.vert.spv", "shaders/pbr.frag.spv"},  // 使用PBR着色器
        m_swapchain->getExtent(),
        m_swapchain->getImageFormat(),
        m_mainRenderPass->getRenderPass(),
        m_descriptorManager->getAllDescriptorSetLayouts()
    );

    // 10. 创建命令管理器（需要swapchain图像数量）
    this->m_commandManager = std::make_unique<CommandManager>(
        m_context.get(),
        MAX_FRAMES_IN_FLIGHT,       //2
        static_cast<uint32_t>(m_swapchain->getImageCount()) // 3
    );
}
std::unique_ptr<RenderPassManager> Renderer::createMainRenderPass(vk::Format color, vk::Format depth) {
    RenderPassConfig forwardConfig;

    // 1. 颜色附件
    forwardConfig.attachments.push_back({
        .type = AttachmentType::Color,
        .format = color,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::ePresentSrcKHR
    });

    // 2. 深度附件
    forwardConfig.attachments.push_back({
        .type = AttachmentType::Depth,
        .format = depth,
        .samples = vk::SampleCountFlagBits::e1,
        .loadOp = vk::AttachmentLoadOp::eClear,
        .storeOp = vk::AttachmentStoreOp::eStore,
        .initialLayout = vk::ImageLayout::eUndefined,
        .finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    });

    // 3. 子通道
    SubpassConfig forwardSubpass{};
    forwardSubpass.colorAttachments.push_back({
        .index = 0,
        .layout = vk::ImageLayout::eColorAttachmentOptimal
    });
    forwardSubpass.depthAttachment = {{
        .index = 1,
        .layout = vk::ImageLayout::eDepthStencilAttachmentOptimal
    }};
    forwardConfig.subpasses.push_back(forwardSubpass);

    return std::make_unique<RenderPassManager>(m_context.get(), forwardConfig);
}

void Renderer::createDepthResources() {
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = m_swapchain->getExtent().width;
    imageInfo.extent.height = m_swapchain->getExtent().height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = vk::Format::eD32Sfloat;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.flags = vk::ImageCreateFlagBits{};
    m_depthImage = m_context->getDevice().createImage(imageInfo);
    // 分配内存
    vk::MemoryRequirements memRequirements = m_context->getDevice().getImageMemoryRequirements(m_depthImage);
    vk::PhysicalDeviceMemoryProperties memProperties = m_context->getPhysicalDevice().getMemoryProperties();
    uint32_t memoryTypeIndex = UINT32_MAX;
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((memRequirements.memoryTypeBits & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
            memoryTypeIndex = i;
            break;
        }
    }
    if (memoryTypeIndex == UINT32_MAX) {
        throw std::runtime_error("failed to find suitable memory type for depth image!");
    }
    vk::MemoryAllocateInfo allocInfo{};
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memoryTypeIndex; // 使用找到的索引

    m_depthImageMemory = m_context->getDevice().allocateMemory(allocInfo);
    m_context->getDevice().bindImageMemory(m_depthImage, m_depthImageMemory, 0);
    // 创建深度图像视图
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = m_depthImage;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = vk::Format::eD32Sfloat;
    viewInfo.components.r = vk::ComponentSwizzle::eIdentity;
    viewInfo.components.g = vk::ComponentSwizzle::eIdentity;
    viewInfo.components.b = vk::ComponentSwizzle::eIdentity;
    viewInfo.components.a = vk::ComponentSwizzle::eIdentity;
    viewInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eDepth;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    m_depthImageView = m_context->getDevice().createImageView(viewInfo);
}


void Renderer::createFramebuffers() {
    const auto& swapchainImageViews = m_swapchain->getImageViews();
    m_swapchainFramebuffers.resize(swapchainImageViews.size());
    auto device = m_context->getDevice();

    for (size_t i = 0; i < swapchainImageViews.size(); ++i) {
        std::array<vk::ImageView, 2> attachments = {
            swapchainImageViews[i],     // 附件 0: 颜色 (来自 Swapchain)
            m_depthImageView            // 附件 1: 深度 (我们自己创建的)
        };

        vk::FramebufferCreateInfo framebufferInfo{};
        framebufferInfo.renderPass = m_mainRenderPass->getRenderPass();  // 使用主渲染通道
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = m_swapchain->getExtent().width;
        framebufferInfo.height = m_swapchain->getExtent().height;
        framebufferInfo.layers = 1;

        try {
            m_swapchainFramebuffers[i] = device.createFramebuffer(framebufferInfo);
        } catch (const vk::SystemError& err) {
            throw std::runtime_error("Failed to create framebuffer!");
        }
    }
}
void Renderer::cleanupUBOs() {
    auto allocator = m_context->getVmaAllocator();

    // 清理帧级
    for (size_t i = 0; i < m_frameAllocations.size(); i++) {
        vmaDestroyBuffer(allocator,
                        static_cast<VkBuffer>(m_frameUBOs.camera[i]),
                        m_frameAllocations[i]);
    }

    // 清理对象级
    for (size_t i = 0; i < m_objectAllocations.size(); i++) {
        size_t objIdx = i / 3;
        size_t typeIdx = i % 3;  // 0=Transform, 1=Light, 2=Material

        vk::Buffer buf;
        switch (typeIdx) {
            case 0: buf = m_objectUBOs.transform[objIdx]; break;
            case 1: buf = m_objectUBOs.light[objIdx]; break;
            case 2: buf = m_objectUBOs.material[objIdx]; break;
        }
        vmaDestroyBuffer(allocator, static_cast<VkBuffer>(buf), m_objectAllocations[i]);
    }
}

void Renderer::updateFrameUBO(uint32_t frame, const CameraUBO& cam) {
    void* data;
    vmaMapMemory(m_context->getVmaAllocator(), m_frameAllocations[frame], &data);
    memcpy(data, &cam, sizeof(CameraUBO));
    vmaUnmapMemory(m_context->getVmaAllocator(), m_frameAllocations[frame]);
}

void Renderer::updateObjectUBO(size_t objIdx, const TransformUBO& trans, const LightUBO& light, const MaterialUBO& mat) {
    // Update Transform (objIdx * 3 + 0)
    void* transData;
    vmaMapMemory(m_context->getVmaAllocator(), m_objectAllocations[objIdx * 3], &transData);
    memcpy(transData, &trans, sizeof(TransformUBO));
    vmaUnmapMemory(m_context->getVmaAllocator(), m_objectAllocations[objIdx * 3]);

    // Update Light (objIdx * 3 + 1)
    void* lightData;
    vmaMapMemory(m_context->getVmaAllocator(), m_objectAllocations[objIdx * 3 + 1], &lightData);
    memcpy(lightData, &light, sizeof(LightUBO));
    vmaUnmapMemory(m_context->getVmaAllocator(), m_objectAllocations[objIdx * 3 + 1]);

    // Update Material (objIdx * 3 + 2)
    void* matData;
    vmaMapMemory(m_context->getVmaAllocator(), m_objectAllocations[objIdx * 3 + 2], &matData);
    memcpy(matData, &mat, sizeof(MaterialUBO));
    vmaUnmapMemory(m_context->getVmaAllocator(), m_objectAllocations[objIdx * 3 + 2]);
}
void Renderer::createFrameUBOs() {
    m_frameUBOs.camera.resize(MAX_FRAMES_IN_FLIGHT);
    m_frameAllocations.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        this->createUniformBuffer<CameraUBO>(
            m_frameUBOs.camera[i], 
            m_frameAllocations[i]
        );
    }
}

void Renderer::createObjectUBOs(size_t objectCount) {
    m_objectUBOs.transform.resize(objectCount);
    m_objectUBOs.light.resize(objectCount);
    m_objectUBOs.material.resize(objectCount);
    m_objectAllocations.resize(objectCount * 3); // Transform + Light + Material

    for (size_t i = 0; i < objectCount; i++) {
        this->createUniformBuffer<TransformUBO>(
            m_objectUBOs.transform[i],
            m_objectAllocations[i * 3]
        );
        this->createUniformBuffer<LightUBO>(
            m_objectUBOs.light[i],
            m_objectAllocations[i * 3 + 1]
        );
        this->createUniformBuffer<MaterialUBO>(
            m_objectUBOs.material[i],
            m_objectAllocations[i * 3 + 2]
        );
    }
}
Renderer::~Renderer() {
    // 0. 确保GPU完成所有工作
    if (m_context && m_context->getDevice()) {
        std::println("Waiting for device idle...");
        m_context->getDevice().waitIdle();
    }
    // ============================================================
    // 按依赖关系逆序清理资源
    // 智能指针会自动清理，但我们按顺序重置以确保正确的销毁顺序
    // ============================================================
    // 1. 清理 PipelineManager (依赖 renderPass, descriptorLayouts)
    m_pipelineManager.reset();
    // 2. 清理 CommandManager (fences, semaphores, command pools)
    m_commandManager.reset();
    // 3. 清理 Framebuffers (依赖 swapchain image views 和 depth image)
    cleanupFramebuffers();
    // 4. 清理深度资源
    cleanupDepthResources();
    // 5. 清理 Swapchain (images, image views)
    m_swapchain.reset();
    // 6. 清理 DescriptorManager (layouts, pool, sets)
    m_descriptorManager.reset();
    // 7. 清理 RenderPass
    m_mainRenderPass.reset();
    // 8. 清理 UBOs (uniform buffers)
    cleanupUBOs();
    // 9. 最后清理 Context (device, instance, VMA)
    m_context.reset();
    std::println("Renderer destruction complete");
}


void Renderer::render(const std::unique_ptr<Scene>& scene) {
    if (m_framebufferResized) {
        this->recreateSwapchainAndDependencies();
        return;
    }
    uint32_t imageIndex;
    uint32_t currentFrame = m_commandManager->getCurrentFrameIndex();

    try{
        imageIndex = m_commandManager->beginFrame(m_swapchain->getSwapchain());
    }catch(const std::exception& e){
        this->recreateSwapchainAndDependencies();
        return;
    }
    
    // 2.更新相机的UBO(set = 0, binding = 0)
    int newWidth = m_swapchain->getExtent().width;
    int newHeight = m_swapchain->getExtent().height;
    scene->getCamera().setViewportSize(static_cast<int>(newWidth), static_cast<int>(newHeight));
    this->updateFrameUBO(currentFrame,scene->getCamera().getUBO()); // 具体的数值应该是在外部更新好了的，这里只复制到显存里面

    // 3. 开始录制命令
    vk::CommandBuffer commandBuffer = m_commandManager->getCurrentCommandBuffer();
    commandBuffer.reset();
    commandBuffer.begin(vk::CommandBufferBeginInfo{});

    // 4. 主渲染通道
    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.setRenderPass(m_mainRenderPass->getRenderPass());

    // 边界检查：确保 imageIndex 在有效范围内
    if (imageIndex >= m_swapchainFramebuffers.size()) {
        std::println("Error: imageIndex {} out of bounds (framebuffer count: {})",imageIndex, m_swapchainFramebuffers.size());
        this->recreateSwapchainAndDependencies();
        return;
    }

    renderPassInfo.setFramebuffer(m_swapchainFramebuffers[imageIndex]);
    renderPassInfo.setRenderArea({ {0, 0}, m_swapchain->getExtent() });
    
    std::array<vk::ClearValue, 2> clearValues{};
    clearValues[0].setColor(std::array<float, 4>{0.02f, 0.02f, 0.02f, 1.0f});
    clearValues[1].setDepthStencil({ 1.0f, 0 });
    renderPassInfo.setClearValues(clearValues);
    
    commandBuffer.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    // 5. 设置动态状态
    vk::Viewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_swapchain->getExtent().width);
    viewport.height = static_cast<float>(m_swapchain->getExtent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    commandBuffer.setViewport(0, viewport);

    vk::Rect2D scissor{};
    scissor.offset = vk::Offset2D{0, 0};
    scissor.extent = m_swapchain->getExtent();
    commandBuffer.setScissor(0, scissor);

    // 6. 遍历场景并录制绘制命令 (优化前)
    // TODO: 在这里按材质/管线分组以优化性能
    for (const auto& renderable : scene->getRenderables()) {
        auto& mesh = renderable->getMesh();
        auto& material = renderable->getMaterial();

        // 更新物体级 UBO (Set 1)
        // TransformUBO (binding 0) - 每个对象每帧更新
        // LightUBO     (binding 1) - 每帧更新 (如果光源会移动)
        // MaterialUBO  (binding 2) - 每帧更新 (材质参数可能动态变化)
        this->updateObjectUBO(
            renderable->getObjectIndex(),
            renderable->getTransform(),
            scene->getMainLight(),
            material.getData()
        );

        PipelineType type = material.getPipelineType();
        commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipelineManager->getPipeline(type));

        vk::Buffer vertexBuffers[] = { mesh.getVertexBuffer() };
        vk::DeviceSize offsets[] = { 0 };
        commandBuffer.bindVertexBuffers(0, 1, vertexBuffers, offsets);
        commandBuffer.bindIndexBuffer(mesh.getIndexBuffer(), 0, vk::IndexType::eUint32);

        // 绑定描述符集
        std::vector<vk::DescriptorSet> descriptorSetsToBind = {
            m_descriptorManager->getDescriptorSet(0, currentFrame),                 // Set 0: 帧级 (Camera)
            m_descriptorManager->getDescriptorSet(1, renderable->getObjectIndex())  // Set 1: 物体级
        };
        commandBuffer.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            m_pipelineManager->getPipelineLayout(type),
            0,
            static_cast<uint32_t>(descriptorSetsToBind.size()),
            descriptorSetsToBind.data(),
            0,
            nullptr
        );
        commandBuffer.drawIndexed(mesh.getIndexCount(), 1, 0, 0, 0);
    }

    commandBuffer.endRenderPass();
    commandBuffer.end();
    try {
        m_commandManager->endFrame(commandBuffer, m_swapchain->getSwapchain());
    } catch (...) {
        this->recreateSwapchainAndDependencies();
    }
}

void Renderer::cleanupFramebuffers(){
    for (auto framebuffer : m_swapchainFramebuffers) {
        m_context->getDevice().destroyFramebuffer(framebuffer,nullptr);
    }
    m_swapchainFramebuffers.clear();
}
void Renderer::cleanupDepthResources() {
    auto device = m_context->getDevice();
    if (m_depthImageView) {
        device.destroyImageView(m_depthImageView);
        m_depthImageView = nullptr;
    }
    if (m_depthImage) {
        device.destroyImage(m_depthImage);
        m_depthImage = nullptr;
    }
    if (m_depthImageMemory) {
        device.freeMemory(m_depthImageMemory);
        m_depthImageMemory = nullptr;
    }
}
void Renderer::recreateSwapchainAndDependencies() {
    std::println("=== Starting swapchain recreation ===");
    // 防止递归调用
    auto device = m_context->getDevice();
    device.waitIdle();
    this->cleanupFramebuffers();
    this->cleanupDepthResources();
    this->m_commandManager.reset();
    this->m_pipelineManager.reset();
    this->m_swapchain->recreate();
    
    // 2.2 重建深度资源（需要新的 swapchain extent）
    this->createDepthResources();
    this->createFramebuffers();
    m_pipelineManager = std::make_unique<PipelineManager>(m_context.get());
    m_pipelineManager->createGraphicsPipeline(
        PipelineType::Main,
        {"shaders/pbr.vert.spv", "shaders/pbr.frag.spv"},
        m_swapchain->getExtent(),
        m_swapchain->getImageFormat(),
        m_mainRenderPass->getRenderPass(),
        m_descriptorManager->getAllDescriptorSetLayouts()
    );
    this->m_commandManager = std::make_unique<CommandManager>(m_context.get(),MAX_FRAMES_IN_FLIGHT,static_cast<uint32_t>(m_swapchain->getImageCount()));
    this->m_framebufferResized = false;
    std::println("=== Stop swapchain recreation ===");
}


void Renderer::waitForIdle() {
    if (m_context && m_context->getDevice()) {
        m_context->getDevice().waitIdle();
    }
}
