#pragma once

#include <vulkan/vulkan.hpp>
#include <memory>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <type_traits>
#include "Core/Context.h"
#include "Scene/UniformBuffer.h"

// Forward declarations for UBO types
struct CameraUBO;
struct TransformUBO;
struct LightUBO;
struct MaterialUBO;

template<typename T>
struct DescriptorTraits {
    static constexpr bool IsValid = false;
    static_assert(DescriptorTraits<T>::IsValid, "Unknown descriptor type used. Please provide a specialization for DescriptorTraits.");
};

// UBO type specializations (value types)
template<>
struct DescriptorTraits<CameraUBO> {
    static constexpr bool IsValid = true;
    static constexpr vk::DescriptorType Type = vk::DescriptorType::eUniformBuffer;
    static constexpr vk::ShaderStageFlags Stages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
};

template<>
struct DescriptorTraits<TransformUBO> {
    static constexpr bool IsValid = true;
    static constexpr vk::DescriptorType Type = vk::DescriptorType::eUniformBuffer;
    static constexpr vk::ShaderStageFlags Stages = vk::ShaderStageFlagBits::eVertex;
};

template<>
struct DescriptorTraits<LightUBO> {
    static constexpr bool IsValid = true;
    static constexpr vk::DescriptorType Type = vk::DescriptorType::eUniformBuffer;
    static constexpr vk::ShaderStageFlags Stages = vk::ShaderStageFlagBits::eFragment;
};

template<>
struct DescriptorTraits<MaterialUBO> {
    static constexpr bool IsValid = true;
    static constexpr vk::DescriptorType Type = vk::DescriptorType::eUniformBuffer;
    static constexpr vk::ShaderStageFlags Stages = vk::ShaderStageFlagBits::eFragment;
};

// Pointer type specializations (for backwards compatibility)
template<typename T>
struct DescriptorTraits<T*> {
    static constexpr bool IsValid = true;
    static constexpr vk::DescriptorType Type = vk::DescriptorType::eUniformBuffer;
    static constexpr vk::ShaderStageFlags Stages = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment;
};

struct TextureSampler {};
template<>
struct DescriptorTraits<TextureSampler> {
    static constexpr bool IsValid = true;
    static constexpr vk::DescriptorType Type = vk::DescriptorType::eCombinedImageSampler;
    static constexpr vk::ShaderStageFlags Stages = vk::ShaderStageFlagBits::eFragment;
};

class DescriptorManager {
private:
    Context* m_context;
    vk::DescriptorPool m_descriptorPool;
    std::unordered_map<uint32_t, vk::DescriptorSetLayout> m_layouts;
    std::unordered_map<uint32_t, std::vector<vk::DescriptorSet>> m_sets;

public:
    explicit DescriptorManager(Context* context);
    ~DescriptorManager();

    // 禁止拷贝和移动
    DescriptorManager(const DescriptorManager&) = delete;
    DescriptorManager& operator=(const DescriptorManager&) = delete;
    DescriptorManager(DescriptorManager&&) = delete;
    DescriptorManager& operator=(DescriptorManager&&) = delete;
    
    template<typename... Types>
    void createLayout(uint32_t setIndex, uint32_t bindStart = 0);

    void createPool(const std::unordered_map<uint32_t, uint32_t>& setCapacity);
    void allocateAllSets(const std::unordered_map<uint32_t, uint32_t>& setCapacity);
    vk::DescriptorSetLayout getLayout(uint32_t setIndex) const;
    vk::DescriptorSet getSet(uint32_t setIndex, uint32_t instanceIndex) const;

    void bindBufferToSet(uint32_t layoutIdx,
                        uint32_t setInstance,
                        uint32_t binding,
                        vk::Buffer buffer,
                        vk::DeviceSize size);

    void bindImageToSet(uint32_t layoutIdx,
                       uint32_t setInstance,
                       uint32_t binding,
                       vk::ImageView imageView,
                       vk::Sampler sampler);

    vk::DescriptorSet getDescriptorSet(uint32_t setIndex, uint32_t index) const;

    std::vector<vk::DescriptorSetLayout> getAllDescriptorSetLayouts() const;
};