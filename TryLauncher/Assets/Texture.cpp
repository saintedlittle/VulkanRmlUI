#include "Texture.h"
#include <iostream>

Texture::Texture(const std::string& path, const AllocatedImage& image, uint32_t width, uint32_t height)
    : m_path(path), m_image(image), m_width(width), m_height(height) {
}

Texture::~Texture() {
    // Note: The AllocatedImage will be cleaned up by the ResourceManager
    // when the AssetManager releases it
}

size_t Texture::GetMemoryUsage() const {
    // Estimate memory usage based on format and dimensions
    size_t bytesPerPixel = 4; // Assume RGBA format
    
    switch (m_image.format) {
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_B8G8R8A8_UNORM:
            bytesPerPixel = 4;
            break;
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_B8G8R8_UNORM:
            bytesPerPixel = 3;
            break;
        case VK_FORMAT_R8G8_UNORM:
            bytesPerPixel = 2;
            break;
        case VK_FORMAT_R8_UNORM:
            bytesPerPixel = 1;
            break;
        default:
            bytesPerPixel = 4; // Default assumption
            break;
    }
    
    size_t baseSize = m_width * m_height * bytesPerPixel;
    
    // Account for mipmaps (approximately 1/3 additional memory)
    if (m_image.mipLevels > 1) {
        baseSize = baseSize * 4 / 3;
    }
    
    return baseSize;
}