#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <string>
#include <print>
#include <stdexcept>
#include <fstream>

#include "Assets/Mesh.h"
#include "Core/Context.h"


enum class PipelineType {
    Main,                   // 主渲染管线
    OPAQUE_GEOMETRY,        // 不透明几何体
    TRANSPARENT_GEOMETRY,   // 透明几何体
    UI,                     // UI 渲染
    SHADOW_CAST             // 阴影投射
};

class PipelineManager {
private:
    Context* m_context;
    std::unordered_map<PipelineType, vk::Pipeline> m_pipelines;
    std::unordered_map<PipelineType, vk::PipelineLayout> m_pipelinelayout;
    vk::ShaderModule createShaderModule(const std::string& filepath);
public:
    explicit PipelineManager(Context* context);
    ~PipelineManager();

    void cleanup();
    // 禁止拷贝和移动
    PipelineManager(const PipelineManager&) = delete;
    PipelineManager& operator=(const PipelineManager&) = delete;
    PipelineManager(PipelineManager&&) = delete;
    PipelineManager& operator=(PipelineManager&&) = delete;

    void createGraphicsPipeline(
        PipelineType type,
        std::vector<std::string> spvPath,
        vk::Extent2D swapchainExtent,
        vk::Format swapchainFormat,
        vk::RenderPass renderPass,
        std::vector<vk::DescriptorSetLayout> setLayouts);

    vk::PipelineLayout getPipelineLayout(PipelineType type);
    vk::Pipeline getPipeline(PipelineType type);
};
