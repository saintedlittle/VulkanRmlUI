#include "AssetManager.h"
#include "Texture.h"
#include "../UI/UIDocument.h"
#include "../Vulkan/VulkanRenderer.h"
#include "../Vulkan/ResourceManager.h"
#include <iostream>
#include <fstream>
#include <filesystem>

AssetManager::AssetManager(VulkanRenderer* renderer, ResourceManager* resourceManager)
    : m_renderer(renderer), m_resourceManager(resourceManager), m_assetBasePath("assets/") {
}

AssetManager::~AssetManager() {
    if (m_initialized) {
        Shutdown();
    }
}

bool AssetManager::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing AssetManager..." << std::endl;
    
    // Get current working directory for debugging
    std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    
    // Verify asset base path exists
    if (!std::filesystem::exists(m_assetBasePath)) {
        std::cout << "Asset base path does not exist: " << std::filesystem::absolute(m_assetBasePath) << std::endl;
        std::cout << "Creating asset base path..." << std::endl;
        std::filesystem::create_directories(m_assetBasePath);
    }
    
    std::cout << "Asset base path: " << std::filesystem::absolute(m_assetBasePath) << std::endl;
    
    m_initialized = true;
    std::cout << "AssetManager initialized successfully" << std::endl;
    return true;
}

void AssetManager::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }
    
    // Clean up expired weak pointers
    for (auto it = m_assetCache.begin(); it != m_assetCache.end();) {
        if (it->second.expired()) {
            it = m_assetCache.erase(it);
        } else {
            ++it;
        }
    }
}

void AssetManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down AssetManager..." << std::endl;
    
    // Unload all assets
    UnloadAllAssets();
    
    m_initialized = false;
    std::cout << "AssetManager shutdown complete" << std::endl;
}

std::shared_ptr<UIDocument> AssetManager::LoadRMLDocument(const std::string& path) {
    // TODO: Implement in task 8
    std::cout << "LoadRMLDocument placeholder: " << path << std::endl;
    return nullptr;
}

bool AssetManager::LoadStylesheet(const std::string& path) {
    if (!m_initialized) {
        std::cerr << "AssetManager not initialized" << std::endl;
        return false;
    }
    
    // Construct full path
    std::string fullPath = m_assetBasePath + path;
    
    // Check if file exists
    if (!std::filesystem::exists(fullPath)) {
        std::cerr << "Stylesheet file not found: " << fullPath << std::endl;
        return false;
    }
    
    // Load stylesheet using RmlUI Factory
    auto stylesheet = Rml::Factory::InstanceStyleSheetFile(fullPath);
    if (stylesheet) {
        std::cout << "Loaded stylesheet: " << path << std::endl;
        return true;
    } else {
        std::cerr << "Failed to load stylesheet: " << fullPath << std::endl;
        return false;
    }
}

std::shared_ptr<Texture> AssetManager::LoadTexture(const std::string& path) {
    if (!m_initialized) {
        std::cerr << "AssetManager not initialized" << std::endl;
        return nullptr;
    }
    
    // Check cache first
    auto it = m_assetCache.find(path);
    if (it != m_assetCache.end()) {
        if (auto existing = it->second.lock()) {
            auto texture = std::dynamic_pointer_cast<Texture>(existing);
            if (texture) {
                std::cout << "Texture loaded from cache: " << path << std::endl;
                return texture;
            }
        }
    }
    
    // Construct full path
    std::string fullPath = m_assetBasePath + path;
    
    // Load image data
    int width, height, channels;
    stbi_uc* pixels = stbi_load(fullPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    
    if (!pixels) {
        std::cerr << "Failed to load texture: " << fullPath << std::endl;
        return nullptr;
    }
    
    // Create Vulkan texture
    if (!m_resourceManager) {
        std::cerr << "ResourceManager not available" << std::endl;
        stbi_image_free(pixels);
        return nullptr;
    }
    
    AllocatedImage image = m_resourceManager->CreateTexture2D(width, height, VK_FORMAT_R8G8B8A8_UNORM);
    if (!image.IsValid()) {
        std::cerr << "Failed to create Vulkan texture for: " << path << std::endl;
        stbi_image_free(pixels);
        return nullptr;
    }
    
    // Upload texture data
    VkDeviceSize imageSize = width * height * 4;
    AllocatedBuffer stagingBuffer = m_resourceManager->CreateStagingBuffer(imageSize);
    
    if (stagingBuffer.IsValid()) {
        // Copy pixel data to staging buffer
        void* data = m_resourceManager->MapBuffer(stagingBuffer);
        if (data) {
            memcpy(data, pixels, imageSize);
            m_resourceManager->UnmapBuffer(stagingBuffer);
        }
        
        // Transition image layout and copy data
        m_resourceManager->TransitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_LAYOUT_UNDEFINED,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
        
        m_resourceManager->CopyBufferToImage(stagingBuffer, image, width, height);
        
        m_resourceManager->TransitionImageLayout(image.image, VK_FORMAT_R8G8B8A8_UNORM,
                                             VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        
        // Cleanup staging buffer
        m_resourceManager->DestroyBuffer(stagingBuffer);
    }
    
    stbi_image_free(pixels);
    
    // Create texture asset
    auto texture = std::make_shared<Texture>(path, image, width, height);
    m_assetCache[path] = texture;
    
    std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;
    return texture;
}

bool AssetManager::LoadFont(const std::string& path, const std::string& name) {
    if (!m_initialized) {
        std::cerr << "AssetManager not initialized" << std::endl;
        return false;
    }
    
    // Construct full path
    std::string fullPath = m_assetBasePath + path;
    
    // Check if file exists
    if (!std::filesystem::exists(fullPath)) {
        std::cerr << "Font file not found: " << fullPath << std::endl;
        return false;
    }
    
    // Load font using RmlUI
    bool success = Rml::LoadFontFace(fullPath);
    if (success) {
        std::cout << "Loaded font: " << path << " as " << name << std::endl;
    } else {
        std::cerr << "Failed to load font: " << fullPath << std::endl;
    }
    
    return success;
}

void AssetManager::UnloadAsset(const std::string& path) {
    auto it = m_assetCache.find(path);
    if (it != m_assetCache.end()) {
        m_assetCache.erase(it);
        std::cout << "Unloaded asset: " << path << std::endl;
    }
}

void AssetManager::UnloadAllAssets() {
    size_t count = m_assetCache.size();
    m_assetCache.clear();
    std::cout << "Unloaded " << count << " assets" << std::endl;
}

size_t AssetManager::GetMemoryUsage() const {
    size_t totalMemory = 0;
    
    for (const auto& pair : m_assetCache) {
        if (auto asset = pair.second.lock()) {
            totalMemory += asset->GetMemoryUsage();
        }
    }
    
    return totalMemory;
}