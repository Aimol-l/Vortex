#include "Core/RenderPass.h"
#include "Core/Context.h"
#include <print>
#include <stdexcept>

vk::RenderPass RenderPassFactory::create(Context* context, const RenderPassConfig& config) {
    // 1. Convert AttachmentConfig → vk::AttachmentDescription
    std::vector<vk::AttachmentDescription> descriptions;
    descriptions.reserve(config.attachments.size());

    // 更完整的深度/模板格式检测
    auto isDepthStencilFormat = [](vk::Format format) {
        switch (format) {
            // 深度+模板格式
            case vk::Format::eD16UnormS8Uint:
            case vk::Format::eD24UnormS8Uint:
            case vk::Format::eD32SfloatS8Uint:
            // 纯深度格式
            case vk::Format::eD16Unorm:
            case vk::Format::eD32Sfloat:
            case vk::Format::eX8D24UnormPack32:
                return true;
            default:
                return false;
        }
    };

    for (const auto& att : config.attachments) {
        vk::AttachmentDescription desc{};
        desc.format = att.format;
        desc.samples = att.samples;
        desc.loadOp = att.loadOp;
        desc.storeOp = att.storeOp;
        desc.initialLayout = att.initialLayout;
        desc.finalLayout = att.finalLayout;

        if (isDepthStencilFormat(att.format)) {
            desc.stencilLoadOp = att.loadOp;
            desc.stencilStoreOp = att.storeOp;
        } else {
            desc.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
            desc.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        }

        descriptions.push_back(desc);
    }

    // Helper: convert vector<AttachmentReference> → vector<vk::AttachmentReference>
    auto toVkRefs = [](const std::vector<AttachmentReference>& refs) {
        std::vector<vk::AttachmentReference> vkRefs;
        vkRefs.reserve(refs.size());
        for (const auto& r : refs) {
            vkRefs.push_back(vk::AttachmentReference{r.index, r.layout});
        }
        return vkRefs;
    };

    // 2. Convert SubpassConfig → vk::SubpassDescription
    std::vector<vk::SubpassDescription> subpassesVk;
    std::vector<std::vector<vk::AttachmentReference>> allColorRefs;
    std::vector<std::vector<vk::AttachmentReference>> allInputRefs;
    std::vector<std::vector<vk::AttachmentReference>> allResolveRefs;
    std::vector<std::optional<vk::AttachmentReference>> allDepthRefs;

    for (const auto& sub : config.subpasses) {
        auto colorRefs = toVkRefs(sub.colorAttachments);
        auto inputRefs = toVkRefs(sub.inputAttachments);

        std::optional<vk::AttachmentReference> depthRef = std::nullopt;
        if (sub.depthAttachment.has_value()) {
            depthRef = vk::AttachmentReference{
                sub.depthAttachment->index,
                sub.depthAttachment->layout
            };
        }

        auto resolveRefs = toVkRefs(sub.resolveAttachments);

        // Validate resolve size
        if (!resolveRefs.empty() && resolveRefs.size() != colorRefs.size()) {
            throw std::runtime_error("Resolve attachments count must match color attachments count if provided.");
        }

        allColorRefs.push_back(std::move(colorRefs));
        allInputRefs.push_back(std::move(inputRefs));
        allDepthRefs.push_back(depthRef);
        allResolveRefs.push_back(std::move(resolveRefs));
    }

    // Now build vk::SubpassDescription
    subpassesVk.reserve(config.subpasses.size());
    for (size_t i = 0; i < config.subpasses.size(); ++i) {
        vk::SubpassDescription sub{};
        sub.pipelineBindPoint = vk::PipelineBindPoint::eGraphics; // Assume graphics for now
        sub.setColorAttachmentCount(static_cast<uint32_t>(allColorRefs[i].size()));
        sub.setPColorAttachments(allColorRefs[i].data());
        sub.setInputAttachmentCount(static_cast<uint32_t>(allInputRefs[i].size()));
        sub.setPInputAttachments(allInputRefs[i].data());

        if (allDepthRefs[i].has_value()) {
            sub.setPDepthStencilAttachment(&allDepthRefs[i].value());
        }

        if (!allResolveRefs[i].empty()) {
            sub.setPResolveAttachments(allResolveRefs[i].data());
        }

        subpassesVk.push_back(sub);
    }

    // 3. Create RenderPass
    vk::RenderPassCreateInfo createInfo{};
    createInfo.setAttachments(descriptions);
    createInfo.setSubpasses(subpassesVk);
    createInfo.setDependencies(config.dependencies);

    try {
        auto renderPass = context->getDevice().createRenderPass(createInfo);
        std::println("RenderPass created successfully with {} attachments and {} subpasses",
                     descriptions.size(), subpassesVk.size());
        return renderPass;
    } catch (const vk::SystemError& err) {
        throw std::runtime_error("Failed to create render pass: " + std::string(err.what()));
    }
}

// RenderPassManager implementation
RenderPassManager::RenderPassManager(Context* context, const RenderPassConfig& config)
    : m_context(context) {
    m_renderPass = RenderPassFactory::create(m_context, config);
}

RenderPassManager::~RenderPassManager() {
    if (m_renderPass) {
        m_context->getDevice().destroyRenderPass(m_renderPass);
        m_renderPass = nullptr;
        std::println("RenderPass destroyed");
    }
}

const vk::RenderPass& RenderPassManager::getRenderPass() const {
    return m_renderPass;
}
