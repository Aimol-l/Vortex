#include "Assets/Material.h"
#include "Core/Descriptor.h"

Material::Material(Context* context,
                   PipelineType pipelineType,
                   const MaterialUBO& uboData,
                   const std::string& albedoPath,
                   const std::string& normalPath,
                   const std::string& metallicPath,
                   const std::string& roughnessPath)
    : m_context(context)
    , m_pipelineType(pipelineType)
    , m_uboData(uboData) {

    // Load textures only if paths are provided
    if (!albedoPath.empty()) {
        m_albedoMap = std::make_unique<Texture>(context, albedoPath);
    }
    if (!normalPath.empty()) {
        m_normalMap = std::make_unique<Texture>(context, normalPath);
    }
    if (!metallicPath.empty()) {
        m_metallicMap = std::make_unique<Texture>(context, metallicPath);
    }
    if (!roughnessPath.empty()) {
        m_roughnessMap = std::make_unique<Texture>(context, roughnessPath);
    }
}

// 移动构造函数和移动赋值运算符使用 = default，在头文件中已声明

void Material::bindToDescriptorSet(DescriptorManager* descriptorManager,
                                  uint32_t layoutIdx,
                                  uint32_t setInstance) {
    // binding 3: albedoMap
    if (m_albedoMap) {
        descriptorManager->bindImageToSet(
            layoutIdx, setInstance, 3,
            m_albedoMap->getImageView(),
            m_albedoMap->getSampler()
        );
    }

    // binding 4: normalMap
    if (m_normalMap) {
        descriptorManager->bindImageToSet(
            layoutIdx, setInstance, 4,
            m_normalMap->getImageView(),
            m_normalMap->getSampler()
        );
    }

    // binding 5: metallicMap
    if (m_metallicMap) {
        descriptorManager->bindImageToSet(
            layoutIdx, setInstance, 5,
            m_metallicMap->getImageView(),
            m_metallicMap->getSampler()
        );
    }

    // binding 6: roughnessMap
    if (m_roughnessMap) {
        descriptorManager->bindImageToSet(
            layoutIdx, setInstance, 6,
            m_roughnessMap->getImageView(),
            m_roughnessMap->getSampler()
        );
    }
}
