#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <string>
#include <stdexcept>
#include "Scene/Camera.h"
#include "Scene/Renderable.h"
#include "Scene/UniformBuffer.h"

// 前向声明，避免在头文件中包含具体实现，减少编译依赖

class Scene {
private:
    std::unique_ptr<Camera> m_camera;
    std::vector<std::shared_ptr<Renderable>> m_renderables;
    std::vector<glm::vec3> m_initialPositions;  // 存储每个物体的初始位置
    LightUBO m_mainLight{
        .position = glm::vec3(2.0f, 2.0f, 2.0f),
        .intensity = 1.0f,
        .color = glm::vec3(1.0f, 1.0f, 1.0f),
        .type = 0  // 点光
    };
    float m_autoRotationAngle = 0.0f;  // 自动旋转角度

public:
    Scene() = default;
    ~Scene() = default;

    // 禁止拷贝和移动（Camera 是 unique_ptr）
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(Scene&&) = delete;

    void setCamera(std::unique_ptr<Camera> camera) {
        m_camera = std::move(camera);
    }

    Camera& getCamera() {
        if (!m_camera) {
            throw std::runtime_error("Camera not set in Scene!");
        }
        return *m_camera;
    }

    const Camera& getCamera() const {
        if (!m_camera) {
            throw std::runtime_error("Camera not set in Scene!");
        }
        return *m_camera;
    }

    void addRenderable(std::shared_ptr<Renderable> object) {
        m_renderables.push_back(object);
        // 自动提取初始位置（从变换矩阵的最后一列）
        auto transform = object->getTransform();
        glm::vec3 position = glm::vec3(transform.model[3]);  // 提取位移分量
        m_initialPositions.push_back(position);
    }

    CameraUBO getCameraData() const {
        if (!m_camera) {
            throw std::runtime_error("Camera not set in Scene!");
        }
        return m_camera->getUBO();
    }

    const std::vector<std::shared_ptr<Renderable>>& getRenderables() const {
        return m_renderables;
    }
    // 光源管理
    void setMainLight(const LightUBO& light) { m_mainLight = light; }
    const LightUBO& getMainLight() const { return m_mainLight; }
    // 自动旋转更新 - 适用于任意数量的物体
    void updateAutoRotation(float deltaTime, float rotationSpeed = 1.0f) {
        m_autoRotationAngle += rotationSpeed * deltaTime;
        // 遍历所有物体，保持各自的初始位置，只添加旋转
        for (size_t i = 0; i < m_renderables.size(); ++i) {
            glm::mat4 modelMatrix = glm::mat4(1.0f);
            // 1. 先移动到初始位置
            modelMatrix = glm::translate(modelMatrix, m_initialPositions[i]);
            // 2. 在该位置旋转
            modelMatrix = glm::rotate(modelMatrix, glm::radians(m_autoRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
            // 3. 应用缩放（默认为1.0）
            modelMatrix = glm::scale(modelMatrix, glm::vec3(1.0f));
            // 更新物体的变换
            m_renderables[i]->updateTransform(modelMatrix);
        }
    }
};
