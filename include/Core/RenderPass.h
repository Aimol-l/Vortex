// RenderPassManager.h
#pragma once

#include <memory>
#include <vector>
#include <optional>
#include <vulkan/vulkan.hpp>
#include "Core/Context.h"

// 附件类型，用于清晰表达意图
enum class AttachmentType {
    Color,
    Depth,
    Resolve, // 用于多重采样
    Preserve // 保留内容，不进行任何操作
};

// 单个附件的描述
struct AttachmentConfig {
    AttachmentType type;
    vk::Format format;
    vk::SampleCountFlagBits samples;
    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR; // 默认值
};
// 子通道对附件的引用
struct AttachmentReference {
    uint32_t index; // 在 AttachmentConfig 数组中的索引
    vk::ImageLayout layout;
};
// 单个子通道的描述
struct SubpassConfig {
    std::vector<AttachmentReference> colorAttachments;
    std::vector<AttachmentReference> inputAttachments;
    std::optional<AttachmentReference> depthAttachment;
    std::vector<AttachmentReference> resolveAttachments;
};
// 完整的 RenderPass 配置
struct RenderPassConfig {
    std::vector<AttachmentConfig> attachments;
    std::vector<SubpassConfig> subpasses;
    std::vector<vk::SubpassDependency> dependencies;
};

class RenderPassFactory {
public:
    // 根据配置创建 vk::RenderPass
    static vk::RenderPass create(Context* context, const RenderPassConfig& config);
};

class RenderPassManager {
private:
    Context* m_context;
    vk::RenderPass m_renderPass;
public:
    ~RenderPassManager();
    RenderPassManager(Context* context, const RenderPassConfig& config);

    // 禁止拷贝和移动
    RenderPassManager(const RenderPassManager&) = delete;
    RenderPassManager& operator=(const RenderPassManager&) = delete;
    RenderPassManager(RenderPassManager&&) = delete;
    RenderPassManager& operator=(RenderPassManager&&) = delete;

    const vk::RenderPass& getRenderPass() const;
};

