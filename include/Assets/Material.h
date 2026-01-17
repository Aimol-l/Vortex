#pragma once

#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <memory>
#include <vector>
#include <string>
#include "Core/Pipeline.h"
#include "Assets/Texture.h"
#include "Core/Descriptor.h"

struct alignas(16) MaterialUBO {
    glm::vec3 albedo{1.0f, 1.0f, 1.0f};
    float metallic{0.0f};
    float roughness{0.5f};
    float ao{1.0f};
    static vk::DescriptorSetLayoutBinding GetBinding(size_t idx) {
        return vk::DescriptorSetLayoutBinding{}
            .setBinding(static_cast<uint32_t>(idx))
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment);
    }
};
class Material {
private:
    Context* m_context;
    PipelineType m_pipelineType;
    MaterialUBO m_uboData;
    std::unique_ptr<Texture> m_albedoMap;
    std::unique_ptr<Texture> m_normalMap;
    std::unique_ptr<Texture> m_metallicMap;
    std::unique_ptr<Texture> m_roughnessMap;
public:
    Material(Context* context,
             PipelineType pipelineType,
             const MaterialUBO& uboData,
             const std::string& albedoPath = "",
             const std::string& normalPath = "",
             const std::string& metallicPath = "",
             const std::string& roughnessPath = "");

    ~Material() = default;
    Material(const Material&) = delete;
    Material& operator=(const Material&) = delete;
    Material(Material&&) = default;
    Material& operator=(Material&&) = default;

    // Pipeline Type
    PipelineType getPipelineType() const { return m_pipelineType; }

    // UBO Data
    MaterialUBO& getData() { return m_uboData;}
    const MaterialUBO& getData() const { return m_uboData; }

    // Textures
    Texture* getAlbedoMap() const { return m_albedoMap.get(); }
    Texture* getNormalMap() const { return m_normalMap.get(); }
    Texture* getMetallicMap() const { return m_metallicMap.get(); }
    Texture* getRoughnessMap() const { return m_roughnessMap.get(); }

    bool hasAlbedoMap() const { return m_albedoMap != nullptr; }
    bool hasNormalMap() const { return m_normalMap != nullptr; }
    bool hasMetallicMap() const { return m_metallicMap != nullptr; }
    bool hasRoughnessMap() const { return m_roughnessMap != nullptr; }

    // Bind all textures to descriptor set
    void bindToDescriptorSet(DescriptorManager* descriptorManager,
                           uint32_t layoutIdx,
                           uint32_t setInstance);
};