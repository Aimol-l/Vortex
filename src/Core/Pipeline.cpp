#include "Core/Pipeline.h"
#include "Pipeline.h"


namespace Utils {
    std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open shader file: " + filename);
        }
        size_t fileSize = (size_t)file.tellg();
        std::vector<char> buffer(fileSize);
        file.seekg(0);
        file.read(buffer.data(), fileSize);
        file.close();
        return buffer;
    }
}

// PipelineManager implementation
PipelineManager::PipelineManager(Context* context)
    : m_context(context) {
    std::println("PipelineManager created");
}

PipelineManager::~PipelineManager() {
    auto device = m_context->getDevice();
    for (auto& [type, pipeline] : m_pipelines) {
        if (pipeline) {
            device.destroyPipeline(pipeline);
        }
    }
    m_pipelines.clear();
    for (auto& [type, layout] : m_pipelinelayout) {
        if (layout) {
            device.destroyPipelineLayout(layout);
        }
    }
    m_pipelinelayout.clear();
    std::println("PipelineManager destroyed");
}
vk::ShaderModule PipelineManager::createShaderModule(const std::string &filepath){
    auto code = Utils::readFile(filepath);
    if (code.size() % 4 != 0) {
        throw std::runtime_error("Shader file size is not a multiple of 4: " + filepath);
    }
    vk::ShaderModuleCreateInfo createInfo{};
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
    try {
        auto shaderModule = m_context->getDevice().createShaderModule(createInfo);
        std::println("Shader module created: {}", filepath);
        return shaderModule;
    } catch (const vk::SystemError& err) {
        throw std::runtime_error("Failed to create shader module from " + filepath + ": " + err.what());
    }
}

void PipelineManager::createGraphicsPipeline(
    PipelineType type,
    std::vector<std::string> spvPath,
    vk::Extent2D swapchainExtent,
    vk::Format swapchainFormat,
    vk::RenderPass renderPass,
    std::vector<vk::DescriptorSetLayout> setLayouts) {

    std::println("Creating graphics pipeline for type {}", static_cast<int>(type));

    vk::ShaderModule vertShaderModule;
    vk::ShaderModule fragShaderModule;

    try {
        vertShaderModule = this->createShaderModule(spvPath[0]);
        fragShaderModule = this->createShaderModule(spvPath[1]);

        // 创建 pipeline layout
        vk::PipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.setSetLayouts(setLayouts);

        m_pipelinelayout[type] = m_context->getDevice().createPipelineLayout(pipelineLayoutInfo);

        // 顶点着色器阶段
        vk::PipelineShaderStageCreateInfo vertexShaderStageInfo{};
        vertexShaderStageInfo.stage = vk::ShaderStageFlagBits::eVertex;
        vertexShaderStageInfo.module = vertShaderModule;
        vertexShaderStageInfo.pName = "main";

        // 片段着色器阶段
        vk::PipelineShaderStageCreateInfo fragmentShaderStageInfo{};
        fragmentShaderStageInfo.stage = vk::ShaderStageFlagBits::eFragment;
        fragmentShaderStageInfo.module = fragShaderModule;
        fragmentShaderStageInfo.pName = "main";

        std::array<vk::PipelineShaderStageCreateInfo, 2> shaderStages = {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        // 顶点输入
        vk::VertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = vk::VertexInputRate::eVertex;

        std::array<vk::VertexInputAttributeDescription, 3> attributeDescriptions;
        // 位置 (location = 0)
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        // 法线 (location = 1)
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = vk::Format::eR32G32B32Sfloat;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);

        // 纹理 (location = 2)
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = vk::Format::eR32G32Sfloat;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        vk::PipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.setVertexBindingDescriptions(bindingDescription)
                   .setVertexAttributeDescriptions(attributeDescriptions);

        // 输入装配（三角形列表）
        vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
        inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // 视口与裁剪
        vk::Viewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapchainExtent.width);
        viewport.height = static_cast<float>(swapchainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        vk::Rect2D scissor{};
        scissor.offset = vk::Offset2D{0, 0};
        scissor.extent = swapchainExtent;

        vk::PipelineViewportStateCreateInfo viewportState{};
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        // 光栅化
        vk::PipelineRasterizationStateCreateInfo rasterizer;
        rasterizer.setDepthClampEnable(false)
                .setRasterizerDiscardEnable(false)
                .setPolygonMode(vk::PolygonMode::eFill)
                .setLineWidth(1.0f)
                .setCullMode(vk::CullModeFlagBits::eNone)
                .setFrontFace(vk::FrontFace::eCounterClockwise)
                .setDepthBiasEnable(false)
                .setDepthBiasConstantFactor(0.0f)
                .setDepthBiasClamp(0.0f)
                .setDepthBiasSlopeFactor(0.0f);

        // 多重采样（禁用）
        vk::PipelineMultisampleStateCreateInfo multisampling;
        multisampling.setSampleShadingEnable(false)
                    .setRasterizationSamples(vk::SampleCountFlagBits::e1)
                    .setMinSampleShading(1.0f)
                    .setAlphaToCoverageEnable(false)
                    .setAlphaToOneEnable(false);

        // 开启深度测试
        vk::PipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.setDepthTestEnable(VK_TRUE)
                .setDepthWriteEnable(VK_TRUE)
                .setDepthCompareOp(vk::CompareOp::eLess)
                .setDepthBoundsTestEnable(VK_FALSE)
                .setStencilTestEnable(VK_FALSE)
                .setMinDepthBounds(0.0f)
                .setMaxDepthBounds(1.0f);

        // 颜色混合（启用）- 修复：添加 Alpha 通道
        vk::PipelineColorBlendAttachmentState colorBlendAttachment;
        colorBlendAttachment.setColorWriteMask(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA)  // ✅ 添加 Alpha 通道
            .setBlendEnable(false)
            .setSrcColorBlendFactor(vk::BlendFactor::eOne)
            .setDstColorBlendFactor(vk::BlendFactor::eZero)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            .setAlphaBlendOp(vk::BlendOp::eAdd);

        vk::PipelineColorBlendStateCreateInfo colorBlending;
        colorBlending.setLogicOpEnable(false)
                    .setLogicOp(vk::LogicOp::eCopy)
                    .setAttachmentCount(1)
                    .setAttachments(colorBlendAttachment);
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        // 创建管线
        vk::GraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = nullptr;
        pipelineInfo.layout = m_pipelinelayout[type];
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = nullptr;

        auto result = m_context->getDevice().createGraphicsPipeline(nullptr, pipelineInfo);
        if (result.result != vk::Result::eSuccess) {
            throw std::runtime_error("Failed to create graphics pipeline!");
        }

        m_pipelines[type] = result.value;

        // ✅ 管线创建后立即销毁 shader modules
        m_context->getDevice().destroyShaderModule(vertShaderModule);
        m_context->getDevice().destroyShaderModule(fragShaderModule);

        std::println("Graphics pipeline created successfully for type {}", static_cast<int>(type));

    } catch (...) {
        // ✅ 异常时也要清理 shader modules
        if (vertShaderModule) {
            m_context->getDevice().destroyShaderModule(vertShaderModule);
        }
        if (fragShaderModule) {
            m_context->getDevice().destroyShaderModule(fragShaderModule);
        }
        throw;
    }
}

vk::PipelineLayout PipelineManager::getPipelineLayout(PipelineType type) {
    return m_pipelinelayout[type];
}

vk::Pipeline PipelineManager::getPipeline(PipelineType type) {
    return m_pipelines[type];
}
