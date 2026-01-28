#pragma once

#include "AssetManager.h"
#include "../Vulkan/ResourceManager.h"
#include <string>

/**
 * Texture asset wrapper for Vulkan image resources.
 * Handles texture loading, format conversion, and GPU resource management.
 */
class Texture : public Asset {
public:
    Texture(const std::string& path, const AllocatedImage& image, uint32_t width, uint32_t height);
    ~Texture();

    // Asset interface
    const std::string& GetPath() const override { return m_path; }
    size_t GetMemoryUsage() const override;

    // Texture properties
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    VkFormat GetFormat() const { return m_image.format; }
    
    // Vulkan resource access
    VkImage GetImage() const { return m_image.image; }
    VkImageView GetImageView() const { return m_image.imageView; }
    const AllocatedImage& GetAllocatedImage() const { return m_image; }

private:
    std::string m_path;
    AllocatedImage m_image;
    uint32_t m_width;
    uint32_t m_height;
};