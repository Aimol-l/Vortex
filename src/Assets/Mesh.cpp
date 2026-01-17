#include "Assets/Mesh.h"


// 定义 tinyobj 实现
#define TINYOBJLOADER_IMPLEMENTATION
#include "3rd/tiny_obj_loader.h"


namespace {
    // Hash function for Vertex to enable unordered_map
    struct VertexHash {
        size_t operator()(const Vertex& vertex) const noexcept {
            size_t h1 = std::hash<float>()(vertex.pos.x);
            size_t h2 = std::hash<float>()(vertex.pos.y);
            size_t h3 = std::hash<float>()(vertex.pos.z);
            size_t h4 = std::hash<float>()(vertex.normal.x);
            size_t h5 = std::hash<float>()(vertex.normal.y);
            size_t h6 = std::hash<float>()(vertex.normal.z);
            size_t h7 = std::hash<float>()(vertex.texCoord.x);
            size_t h8 = std::hash<float>()(vertex.texCoord.y);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5) ^ (h7 << 6) ^ (h8 << 7);
        }
    };
}

// Load from OBJ file constructor
Mesh::Mesh(Context* context, const std::string& objPath)
    : m_context(context)
    , m_indexCount(0)
    , m_vertexBuffer(nullptr)
    , m_indexBuffer(nullptr)
    , m_vertexAllocation(VK_NULL_HANDLE)
    , m_indexAllocation(VK_NULL_HANDLE) {

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    // Load OBJ file (triangulate = true, mtl_basedir = NULL 不加载 MTL)
    bool triangulate = true;
    const char* mtl_basedir = nullptr;  // 不加载 MTL 文件

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objPath.c_str(), mtl_basedir, triangulate)) {
        throw std::runtime_error("Failed to load OBJ file: " + objPath + "\n" + err);
    }

    // 只打印严重错误，忽略 MTL 相关警告
    if (!err.empty()) {
        std::cerr << "OBJ loading error: " << err << std::endl;
    }

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::unordered_map<Vertex, uint32_t, VertexHash> uniqueVertices{};

    // Process each shape in the OBJ file
    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex{};

            // Position (location = 0)
            vertex.pos = {
                attrib.vertices[3 * index.vertex_index + 0],
                attrib.vertices[3 * index.vertex_index + 1],
                attrib.vertices[3 * index.vertex_index + 2]
            };

            // Normal (location = 1)
            if (index.normal_index >= 0) {
                vertex.normal = {
                    attrib.normals[3 * index.normal_index + 0],
                    attrib.normals[3 * index.normal_index + 1],
                    attrib.normals[3 * index.normal_index + 2]
                };
            } else {
                vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f); // Default normal
            }

            // Texture coordinate (location = 2)
            if (index.texcoord_index >= 0) {
                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    attrib.texcoords[2 * index.texcoord_index + 1]
                };
            } else {
                vertex.texCoord = glm::vec2(0.0f, 0.0f); // Default UV
            }

            // Deduplicate vertices using hash map
            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }
            indices.push_back(uniqueVertices[vertex]);
        }
    }

    m_indexCount = static_cast<uint32_t>(indices.size());

    // Create Vulkan buffers
    this->createVertexBuffer(vertices);
    this->createIndexBuffer(indices);

    std::println("Loaded OBJ:{} - Vertices:{},Indices:{}",objPath,vertices.size(),indices.size());
}

// Destructor
Mesh::~Mesh() {
    if (m_context && m_vertexBuffer && m_vertexAllocation != VK_NULL_HANDLE) {
        auto allocator = m_context->getVmaAllocator();
        vmaDestroyBuffer(allocator, static_cast<VkBuffer>(m_vertexBuffer), m_vertexAllocation);
    }
    if (m_context && m_indexBuffer && m_indexAllocation != VK_NULL_HANDLE) {
        auto allocator = m_context->getVmaAllocator();
        vmaDestroyBuffer(allocator, static_cast<VkBuffer>(m_indexBuffer), m_indexAllocation);
    }
}

// Move constructor
Mesh::Mesh(Mesh&& other) noexcept
    : m_context(other.m_context)
    , m_indexCount(other.m_indexCount)
    , m_vertexBuffer(other.m_vertexBuffer)
    , m_indexBuffer(other.m_indexBuffer)
    , m_vertexAllocation(other.m_vertexAllocation)
    , m_indexAllocation(other.m_indexAllocation) {
    // Reset source object
    other.m_vertexBuffer = nullptr;
    other.m_indexBuffer = nullptr;
    other.m_vertexAllocation = VK_NULL_HANDLE;
    other.m_indexAllocation = VK_NULL_HANDLE;
}

// Move assignment operator
Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        // Destroy current resources
        if (m_context) {
            auto allocator = m_context->getVmaAllocator();
            if (m_vertexBuffer && m_vertexAllocation != VK_NULL_HANDLE) {
                vmaDestroyBuffer(allocator, static_cast<VkBuffer>(m_vertexBuffer), m_vertexAllocation);
            }
            if (m_indexBuffer && m_indexAllocation != VK_NULL_HANDLE) {
                vmaDestroyBuffer(allocator, static_cast<VkBuffer>(m_indexBuffer), m_indexAllocation);
            }
        }

        // Transfer ownership
        m_context = other.m_context;
        m_indexCount = other.m_indexCount;
        m_vertexBuffer = other.m_vertexBuffer;
        m_indexBuffer = other.m_indexBuffer;
        m_vertexAllocation = other.m_vertexAllocation;
        m_indexAllocation = other.m_indexAllocation;

        // Reset source object
        other.m_vertexBuffer = nullptr;
        other.m_indexBuffer = nullptr;
        other.m_vertexAllocation = VK_NULL_HANDLE;
        other.m_indexAllocation = VK_NULL_HANDLE;
    }
    return *this;
}

// Create vertex buffer from vertex data
void Mesh::createVertexBuffer(const std::vector<Vertex>& vertices) {
    vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    // 1. Create staging buffer (CPU visible)
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        stagingBuffer,
        stagingAllocation
    );

    // 2. Copy data to staging buffer
    void* data;
    vmaMapMemory(m_context->getVmaAllocator(), stagingAllocation, &data);
    memcpy(data, vertices.data(), static_cast<size_t>(bufferSize));
    vmaUnmapMemory(m_context->getVmaAllocator(), stagingAllocation);

    // 3. Create device-local vertex buffer
    VkBuffer vertexBuffer;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        vertexBuffer,
        m_vertexAllocation
    );
    m_vertexBuffer = vertexBuffer;

    // 4. Execute copy from staging to device-local buffer
    copyBuffer(stagingBuffer, static_cast<vk::Buffer>(m_vertexBuffer), bufferSize);

    // 5. Cleanup staging buffer
    vmaDestroyBuffer(m_context->getVmaAllocator(), stagingBuffer, stagingAllocation);
}

// Create index buffer from index data
void Mesh::createIndexBuffer(const std::vector<uint32_t>& indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    // 1. Create staging buffer
    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_ONLY,
        stagingBuffer,
        stagingAllocation
    );

    // 2. Copy data to staging buffer
    void* data;
    vmaMapMemory(m_context->getVmaAllocator(), stagingAllocation, &data);
    memcpy(data, indices.data(), static_cast<size_t>(bufferSize));
    vmaUnmapMemory(m_context->getVmaAllocator(), stagingAllocation);

    // 3. Create device-local index buffer
    VkBuffer indexBuffer;
    createBuffer(
        bufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_GPU_ONLY,
        indexBuffer,
        m_indexAllocation
    );
    m_indexBuffer = indexBuffer;

    // 4. Execute copy
    copyBuffer(stagingBuffer, static_cast<vk::Buffer>(m_indexBuffer), bufferSize);

    // 5. Cleanup staging buffer
    vmaDestroyBuffer(m_context->getVmaAllocator(), stagingBuffer, stagingAllocation);
}

// Create a Vulkan buffer with specified properties
void Mesh::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage,
                        VkBuffer& buffer, VmaAllocation& allocation) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;

    VkResult result = vmaCreateBuffer(
        m_context->getVmaAllocator(),
        &bufferInfo,
        &allocInfo,
        &buffer,
        &allocation,
        nullptr
    );

    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create buffer");
    }
}

// Copy data from srcBuffer to dstBuffer using command buffer
void Mesh::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, VkDeviceSize size) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::BufferCopy copyRegion{};
    copyRegion.size = size;
    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

// Begin a one-time submit command buffer
vk::CommandBuffer Mesh::beginSingleTimeCommands() {
    vk::CommandBufferAllocateInfo allocInfo{};
    allocInfo.level = vk::CommandBufferLevel::ePrimary;
    allocInfo.commandPool = m_context->getTransientCommandPool();
    allocInfo.commandBufferCount = 1;

    vk::CommandBuffer commandBuffer = m_context->getDevice().allocateCommandBuffers(allocInfo)[0];

    vk::CommandBufferBeginInfo beginInfo{};
    beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    commandBuffer.begin(beginInfo);

    return commandBuffer;
}

// End and submit a one-time command buffer
void Mesh::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    auto graphicsQueue = m_context->getGraphicsQueue();
    auto res = graphicsQueue.submit(1, &submitInfo, nullptr);
    graphicsQueue.waitIdle();

    m_context->getDevice().freeCommandBuffers(m_context->getTransientCommandPool(), 1, &commandBuffer);
}
