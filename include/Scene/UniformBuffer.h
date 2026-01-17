#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <memory>
#include <vulkan/vulkan.hpp>


struct CameraUBO{
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 projection;
    alignas(16) glm::vec3 position;  // 用于光照计算
    static vk::DescriptorSetLayoutBinding GetBinding(size_t idx){
        vk::DescriptorSetLayoutBinding layoutBinding{};
        layoutBinding.binding = idx;                                        // 对应着色器中的 layout(binding = 0)
        layoutBinding.descriptorType = vk::DescriptorType::eUniformBuffer;  // 这是一个 UBO
        layoutBinding.descriptorCount = 1;                                  // 只有一个描述符（一个 UBO）
        layoutBinding.stageFlags = vk::ShaderStageFlagBits::eVertex;        // 在顶点着色器阶段使用
        layoutBinding.pImmutableSamplers = nullptr;                         // 对于 UBO，这是 nullptr
        return layoutBinding;
    }
    CameraUBO(){
        view = glm::mat4(1.0f);
        projection = glm::mat4(1.0f);
    }
    CameraUBO(glm::mat4 viewMatrix, glm::mat4 projMatrix,glm::vec3 pos)
        : view(viewMatrix), projection(projMatrix),position(pos) {}
};
struct TransformUBO {
    alignas(16) glm::mat4 model;        // 模型矩阵
    alignas(16) glm::mat4 normalMatrix; // 法线矩阵（用于光照）

    static vk::DescriptorSetLayoutBinding GetBinding(size_t idx) {
        return vk::DescriptorSetLayoutBinding{}
            .setBinding(static_cast<uint32_t>(idx))
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eVertex); // 顶点着色器使用
    }
};
struct LightUBO {
    alignas(16) glm::vec3 position;  // 16字节对齐（vec3 实际占12字节，但UBO要求vec4对齐）
    alignas(4)  float intensity;
    alignas(16) glm::vec3 color;
    alignas(4)  uint32_t type;       // 0=点光, 1=方向光

    static vk::DescriptorSetLayoutBinding GetBinding(size_t idx) {
        return vk::DescriptorSetLayoutBinding{}
            .setBinding(static_cast<uint32_t>(idx))
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment); // 光照通常在片元着色器计算
    }
};

struct SceneParams {
    alignas(16) glm::vec3 cameraPos;
    alignas(4)  float deltaTime;
    alignas(4)  uint32_t frameCount;
    alignas(4)  float exposure;

    static vk::DescriptorSetLayoutBinding GetBinding(size_t idx) {
        return vk::DescriptorSetLayoutBinding{}
            .setBinding(static_cast<uint32_t>(idx))
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setStageFlags(
                vk::ShaderStageFlagBits::eVertex | 
                vk::ShaderStageFlagBits::eFragment // 顶点和片元都需要
            );
    }
};