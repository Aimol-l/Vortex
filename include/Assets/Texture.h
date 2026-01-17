#pragma once

#include <string>
#include <stdexcept>
#include <iostream>
#include <vulkan/vulkan.hpp>
#include "Core/Context.h"

class Texture {
private:
    Context* m_context;
    uint32_t m_mipLevels;
    uint32_t m_width;
    uint32_t m_height;

    vk::Image m_image;
    vk::ImageView m_imageView;
    vk::Sampler m_sampler;
    VmaAllocation m_allocation;

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels,
                     vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage,
                     VmaMemoryUsage memoryUsage);
    void createImageView(vk::Format format, vk::ImageAspectFlags aspectFlags);
    void createSampler();
    void transitionImageLayout(vk::Image image, vk::Format format, vk::ImageLayout oldLayout,
                              vk::ImageLayout newLayout, uint32_t mipLevels);
    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
    void generateMipmaps(vk::Image image, vk::Format format, int32_t texWidth,
                         int32_t texHeight, uint32_t mipLevels);
    vk::CommandBuffer beginSingleTimeCommands();
    void endSingleTimeCommands(vk::CommandBuffer commandBuffer);

public:
    ~Texture();
    Texture(Context* context, const std::string& filepath);

    // Disable copying
    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    // Getters
    vk::ImageView getImageView() const { return m_imageView; }
    vk::Sampler getSampler() const { return m_sampler; }
    vk::Image getImage() const { return m_image; }
    uint32_t getMipLevels() const { return m_mipLevels; }
};



