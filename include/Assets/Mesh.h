#pragma once

#include <vector>
#include <memory>
#include <print>
#include <iostream>
#include <unordered_map>
#include <glm/glm.hpp>
#include <vulkan/vulkan.hpp>
#include "Core/Context.h"

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    bool operator==(const Vertex& other) const {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
    }
    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription{
            0,                          // binding
            sizeof(Vertex),             // stride
            vk::VertexInputRate::eVertex
        };
    }
    static std::array<vk::VertexInputAttributeDescription, 3> getAttributeDescriptions() {
        return std::array{
            vk::VertexInputAttributeDescription{
                0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)
            },
            vk::VertexInputAttributeDescription{
                1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)
            },
            vk::VertexInputAttributeDescription{
                2, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)
            }
        };
    }
};

// 读取obj文件为顶点和索引（保存到显存上）
class Mesh {
private:
    Context* m_context;
    uint32_t m_indexCount;
    vk::Buffer m_indexBuffer;
    vk::Buffer m_vertexBuffer;
    VmaAllocation m_indexAllocation;
    VmaAllocation m_vertexAllocation;

    void createVertexBuffer(const std::vector<Vertex>& vertices);
    void createIndexBuffer(const std::vector<uint32_t>& indices);
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VkBuffer& buffer, VmaAllocation& allocation);
    void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, VkDeviceSize size);
    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

public:
    // Default constructor
    Mesh(Context* context)
        : m_context(context)
        , m_indexCount(0)
        , m_vertexBuffer(nullptr)
        , m_indexBuffer(nullptr)
        , m_vertexAllocation(VK_NULL_HANDLE)
        , m_indexAllocation(VK_NULL_HANDLE) {}

    // Load from OBJ file
    Mesh(Context* context, const std::string& objPath);
    Mesh(Context* context, const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices)
        : m_context(context), m_indexCount(static_cast<uint32_t>(indices.size())) {
        this->createVertexBuffer(vertices);
        this->createIndexBuffer(indices);
    }

    ~Mesh();
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    // Getters
    vk::Buffer getVertexBuffer() const { return m_vertexBuffer; }
    vk::Buffer getIndexBuffer() const { return m_indexBuffer; }
    uint32_t getIndexCount() const { return m_indexCount; }
};
