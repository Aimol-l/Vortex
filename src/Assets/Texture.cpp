#include "Assets/Texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "3rd/stb_image.h"

// Constructor: Load texture from file
Texture::Texture(Context* context, const std::string& filepath)
    : m_context(context)
    , m_mipLevels(0)
    , m_width(0)
    , m_height(0)
    , m_allocation(VK_NULL_HANDLE) {

    // Load image from file using stb_image
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load(filepath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

    if (!pixels) {
        throw std::runtime_error("Failed to load texture image: " + filepath);
    }

    m_width = static_cast<uint32_t>(texWidth);
    m_height = static_cast<uint32_t>(texHeight);

    // ⚠️ 关键修复：翻转Y轴以匹配Vulkan的坐标系
    // stb_image加载的图像原点在左上角，Vulkan期望原点在左下角
    stbi_uc* flippedPixels = new stbi_uc[texWidth * texHeight * 4];
    for (int y = 0; y < texHeight; y++) {
        for (int x = 0; x < texWidth; x++) {
            int srcIdx = (y * texWidth + x) * 4;
            int dstIdx = ((texHeight - 1 - y) * texWidth + x) * 4;
            flippedPixels[dstIdx + 0] = pixels[srcIdx + 0]; // R
            flippedPixels[dstIdx + 1] = pixels[srcIdx + 1]; // G
            flippedPixels[dstIdx + 2] = pixels[srcIdx + 2]; // B
            flippedPixels[dstIdx + 3] = pixels[srcIdx + 3]; // A
        }
    }

    // Calculate mip levels
    m_mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(m_width, m_height)))) + 1;

    vk::DeviceSize imageSize = m_width * m_height * 4; // 4 bytes per pixel (RGBA)

    // Create staging buffer using VMA
    vk::BufferCreateInfo bufferInfo{};
    bufferInfo.size = imageSize;
    bufferInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    bufferInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = VMA_MEMORY_USAGE_CPU_ONLY;

    VkBuffer stagingBuffer;
    VmaAllocation stagingAllocation;

    VkResult result = vmaCreateBuffer(m_context->getVmaAllocator(),
                                      reinterpret_cast<VkBufferCreateInfo*>(&bufferInfo),
                                      &allocInfo,
                                      &stagingBuffer,
                                      &stagingAllocation,
                                      nullptr);

    if (result != VK_SUCCESS) {
        stbi_image_free(pixels);
        throw std::runtime_error("Failed to create staging buffer for texture");
    }

    // Copy image data to staging buffer（使用翻转后的像素）
    void* data;
    vmaMapMemory(m_context->getVmaAllocator(), stagingAllocation, &data);
    memcpy(data, flippedPixels, static_cast<size_t>(imageSize));
    vmaUnmapMemory(m_context->getVmaAllocator(), stagingAllocation);

    // 释放原始和翻转后的像素数据
    delete[] flippedPixels;
    stbi_image_free(pixels);

    // Create texture image
    createImage(m_width, m_height, m_mipLevels,
                vk::Format::eR8G8B8A8Srgb,
                vk::ImageTiling::eOptimal,
                vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled,
                VMA_MEMORY_USAGE_GPU_ONLY);

    // Transition image layout and copy buffer to image
    transitionImageLayout(m_image, vk::Format::eR8G8B8A8Srgb,
                         vk::ImageLayout::eUndefined,
                         vk::ImageLayout::eTransferDstOptimal, m_mipLevels);

    copyBufferToImage(static_cast<vk::Buffer>(stagingBuffer), m_image, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));

    // Cleanup staging buffer
    vmaDestroyBuffer(m_context->getVmaAllocator(), stagingBuffer, stagingAllocation);

    // Generate mipmaps
    generateMipmaps(m_image, vk::Format::eR8G8B8A8Srgb, texWidth, texHeight, m_mipLevels);

    // Create image view
    createImageView(vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);

    // Create sampler
    createSampler();

    std::cout << "Texture loaded: " << filepath << " (" << m_width << "x" << m_height
              << ", " << m_mipLevels << " mip levels)" << std::endl;
}

// Destructor
Texture::~Texture() {
    if (m_context) {
        if (m_sampler) {
            m_context->getDevice().destroySampler(m_sampler);
        }
        if (m_imageView) {
            m_context->getDevice().destroyImageView(m_imageView);
        }
        if (m_image && m_allocation != VK_NULL_HANDLE) {
            vmaDestroyImage(m_context->getVmaAllocator(), static_cast<VkImage>(m_image), m_allocation);
        }
    }
}

// Create Vulkan image
void Texture::createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                          vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                          VmaMemoryUsage memoryUsage) {
    vk::ImageCreateInfo imageInfo{};
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;
    imageInfo.usage = usage;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;

    VmaAllocationCreateInfo allocInfo{};
    allocInfo.usage = memoryUsage;

    VkImage image;
    if (vmaCreateImage(m_context->getVmaAllocator(),
                       reinterpret_cast<VkImageCreateInfo*>(&imageInfo),
                       &allocInfo,
                       &image,
                       &m_allocation,
                       nullptr) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create image");
    }
    m_image = image;
}

// Create image view
void Texture::createImageView(vk::Format format, vk::ImageAspectFlags aspectFlags) {
    vk::ImageViewCreateInfo viewInfo{};
    viewInfo.image = m_image;
    viewInfo.viewType = vk::ImageViewType::e2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = m_mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    m_imageView = m_context->getDevice().createImageView(viewInfo);
    if (!m_imageView) {
        throw std::runtime_error("Failed to create texture image view");
    }
}

// Create texture sampler
void Texture::createSampler() {
    vk::PhysicalDeviceProperties properties = m_context->getPhysicalDevice().getProperties();

    vk::SamplerCreateInfo samplerInfo{};
    samplerInfo.magFilter = vk::Filter::eLinear;
    samplerInfo.minFilter = vk::Filter::eLinear;
    samplerInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = vk::CompareOp::eAlways;
    samplerInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(m_mipLevels);

    m_sampler = m_context->getDevice().createSampler(samplerInfo);
    if (!m_sampler) {
        throw std::runtime_error("Failed to create texture sampler");
    }
}

// Transition image layout
void Texture::transitionImageLayout(vk::Image image, vk::Format format,
                                   vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
                                   uint32_t mipLevels) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier{};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    vk::PipelineStageFlags sourceStage;
    vk::PipelineStageFlags destinationStage;

    if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
        barrier.srcAccessMask = {};
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
        destinationStage = vk::PipelineStageFlagBits::eTransfer;
    } else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        sourceStage = vk::PipelineStageFlagBits::eTransfer;
        destinationStage = vk::PipelineStageFlagBits::eFragmentShader;
    } else {
        throw std::invalid_argument("Unsupported layout transition");
    }

    commandBuffer.pipelineBarrier(sourceStage, destinationStage, {}, {}, {}, barrier);

    endSingleTimeCommands(commandBuffer);
}

// Copy buffer to image
void Texture::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::BufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = vk::Offset3D{0, 0, 0};
    region.imageExtent = vk::Extent3D{width, height, 1};

    commandBuffer.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);

    endSingleTimeCommands(commandBuffer);
}

// Generate mipmaps
void Texture::generateMipmaps(vk::Image image, vk::Format format, int32_t texWidth,
                              int32_t texHeight, uint32_t mipLevels) {
    vk::CommandBuffer commandBuffer = beginSingleTimeCommands();

    vk::ImageMemoryBarrier barrier{};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                     vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, barrier);

        vk::ImageBlit blit{};
        blit.srcOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.srcOffsets[1] = vk::Offset3D{mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D{0, 0, 0};
        blit.dstOffsets[1] = vk::Offset3D{mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal,
                               image, vk::ImageLayout::eTransferDstOptimal,
                               blit, vk::Filter::eLinear);

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                     vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                 vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, barrier);

    endSingleTimeCommands(commandBuffer);
}

// Begin single time command buffer
vk::CommandBuffer Texture::beginSingleTimeCommands() {
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

// End single time command buffer
void Texture::endSingleTimeCommands(vk::CommandBuffer commandBuffer) {
    commandBuffer.end();

    vk::SubmitInfo submitInfo{};
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    m_context->getGraphicsQueue().submit(submitInfo, nullptr);
    m_context->getGraphicsQueue().waitIdle();

    m_context->getDevice().freeCommandBuffers(m_context->getTransientCommandPool(), commandBuffer);
}
