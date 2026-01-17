#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Scene/UniformBuffer.h"

class Camera {
private:
    glm::vec3 m_position{0.0f, 0.0f, 3.0f};
    glm::vec3 m_front{0.0f, 0.0f, -1.0f};
    glm::vec3 m_up{0.0f, 1.0f, 0.0f};
    glm::vec3 m_right{1.0f, 0.0f, 0.0f};

    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    float m_movementSpeed = 2.5f;
    float m_mouseSensitivity = 0.1f;

    float m_fov = 45.0f;
    float m_near = 0.1f;
    float m_far = 100.0f;

    int m_width = 1280;
    int m_height = 720;

    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        front.y = sin(glm::radians(m_pitch));
        front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
        m_front = glm::normalize(front);
        m_right = glm::normalize(glm::cross(m_front, glm::vec3(0.0f, 1.0f, 0.0f)));
        m_up = glm::normalize(glm::cross(m_right, m_front));
    }

public:
    Camera() {
        // 初始化相机向量
        updateCameraVectors();
    }
    ~Camera() = default;

    // 禁止拷贝和移动
    Camera(const Camera&) = delete;
    Camera& operator=(const Camera&) = delete;
    Camera(Camera&&) = delete;
    Camera& operator=(Camera&&) = delete;

    void setPosition(const glm::vec3& position) {
        m_position = position;
    }

    glm::vec3 getPosition() const {
        return m_position;
    }

    void setViewportSize(int width, int height) {
        m_width = width;
        m_height = height;
    }

    void moveForward(float deltaTime) {
        float velocity = m_movementSpeed * deltaTime;
        m_position += m_front * velocity;
    }

    void moveBack(float deltaTime) {
        float velocity = m_movementSpeed * deltaTime;
        m_position -= m_front * velocity;
    }

    void moveLeft(float deltaTime) {
        float velocity = m_movementSpeed * deltaTime;
        m_position -= m_right * velocity;
    }

    void moveRight(float deltaTime) {
        float velocity = m_movementSpeed * deltaTime;
        m_position += m_right * velocity;
    }

    void moveUp(float deltaTime) {
        float velocity = m_movementSpeed * deltaTime;
        m_position += m_up * velocity;
    }

    void moveDown(float deltaTime) {
        float velocity = m_movementSpeed * deltaTime;
        m_position -= m_up * velocity;
    }

    void rotate(float deltaX, float deltaY) {
        m_yaw += deltaX * m_mouseSensitivity;
        m_pitch += deltaY * m_mouseSensitivity;
        if (m_pitch > 89.0f) m_pitch = 89.0f;
        if (m_pitch < -89.0f) m_pitch = -89.0f;
        updateCameraVectors();
    }

    glm::mat4 getViewMatrix() const {
        return glm::lookAt(m_position, m_position + m_front, m_up);
    }

    glm::mat4 getProjectionMatrix() const {
        return glm::perspective(glm::radians(m_fov),
                               static_cast<float>(m_width) / static_cast<float>(m_height),
                               m_near, m_far);
    }

    CameraUBO getUBO() const {
        return CameraUBO(getViewMatrix(),getProjectionMatrix(),m_position);
    }
};
