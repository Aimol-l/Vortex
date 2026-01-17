#pragma once

#include <memory>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "Scene/UniformBuffer.h"
#include "Assets/Material.h"
#include "Assets/Mesh.h"

class Renderable {
private:
    uint32_t m_objectIndex;
    TransformUBO m_transform;
    std::shared_ptr<Mesh> m_mesh;
    std::shared_ptr<Material> m_material;

public:
    Renderable(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material, uint32_t objectIndex)
        : m_mesh(std::move(mesh))
        , m_material(std::move(material))
        , m_objectIndex(objectIndex) {
        updateTransform(glm::mat4(1.0f));
    }

    ~Renderable() = default;
    Renderable(Renderable&&) = default;
    Renderable(const Renderable&) = delete;
    Renderable& operator=(Renderable&&) = default;
    Renderable& operator=(const Renderable&) = delete;

    void updateTransform(const glm::mat4& modelMatrix) {
        m_transform.model = modelMatrix;
        // 法线矩阵 = 模型矩阵的逆转置（用于正确变换法线）
        glm::mat3 normalMat3 = glm::transpose(glm::inverse(glm::mat3(modelMatrix)));
        m_transform.normalMatrix = glm::mat4(normalMat3);
    }

    Mesh& getMesh() const { return *m_mesh; }
    Material& getMaterial() const { return *m_material; }
    uint32_t getObjectIndex() const { return m_objectIndex; }
    const TransformUBO& getTransform() const { return m_transform; }
};
