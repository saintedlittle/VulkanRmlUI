#pragma once

#include "../Engine/Engine.h"
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <cstdint>
#include <vector>
class VulkanDevice;
class VulkanRenderer;

/**
 * AllocatedBuffer represents a Vulkan buffer with its associated VMA allocation.
 * This structure encapsulates both the buffer handle and memory management information.
 */
struct AllocatedBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    
    // Utility methods
    bool IsValid() const { return buffer != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE; }
    void* GetMappedData() const { return info.pMappedData; }
    VkDeviceSize GetSize() const { return info.size; }
};

/**
 * AllocatedImage represents a Vulkan image with its associated VMA allocation and image view.
 * This structure encapsulates the image, its memory, and view for rendering operations.
 */
struct AllocatedImage {
    VkImage image = VK_NULL_HANDLE;
    VkImageView imageView = VK_NULL_HANDLE;
    VmaAllocation allocation = VK_NULL_HANDLE;
    VmaAllocationInfo info = {};
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkExtent3D extent = {};
    uint32_t mipLevels = 1;
    
    // Utility methods
    bool IsValid() const { return image != VK_NULL_HANDLE && allocation != VK_NULL_HANDLE; }
    void* GetMappedData() const { return info.pMappedData; }
    VkDeviceSize GetSize() const { return info.size; }
};

/**
 * ResourceManager provides efficient Vulkan resource management using VMA (Vulkan Memory Allocator).
 * It handles buffer and image creation, memory allocation, and resource lifecycle management.
 * 
 * Key features:
 * - VMA integration for optimal memory allocation
 * - Buffer creation for vertex, index, and uniform data
 * - Image creation with format conversion and mipmap support
 * - Staging operations for efficient data transfer
 * - Resource tracking and automatic cleanup
 */
class ResourceManager : public IEngineModule {
public:
    struct InitInfo {
        VulkanRenderer* renderer = nullptr;
    };
    
    ResourceManager(VulkanRenderer* renderer);
    ~ResourceManager();
    
    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "ResourceManager"; }
    int GetInitializationOrder() const override { return 350; } // After VulkanRenderer (300)
    
    // Initialization and cleanup
    void Cleanup();
    
    // Buffer creation methods
    AllocatedBuffer CreateBuffer(VkDeviceSize size, 
                                VkBufferUsageFlags usage, 
                                VmaMemoryUsage memoryUsage,
                                VmaAllocationCreateFlags flags = 0);
    
    // Specialized buffer creation methods
    AllocatedBuffer CreateVertexBuffer(VkDeviceSize size, bool hostVisible = false);
    AllocatedBuffer CreateIndexBuffer(VkDeviceSize size, bool hostVisible = false);
    AllocatedBuffer CreateUniformBuffer(VkDeviceSize size);
    AllocatedBuffer CreateStagingBuffer(VkDeviceSize size);
    
    // Image creation methods
    AllocatedImage CreateImage(const VkImageCreateInfo& imageInfo, 
                              VmaMemoryUsage memoryUsage,
                              VmaAllocationCreateFlags flags = 0);
    
    AllocatedImage CreateImage2D(uint32_t width, 
                                uint32_t height, 
                                VkFormat format,
                                VkImageUsageFlags usage,
                                uint32_t mipLevels = 1,
                                VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);
    
    AllocatedImage CreateTexture2D(uint32_t width,
                                  uint32_t height,
                                  VkFormat format,
                                  uint32_t mipLevels = 1);
    
    // Resource destruction
    void DestroyBuffer(const AllocatedBuffer& buffer);
    void DestroyImage(const AllocatedImage& image);
    
    // Memory mapping utilities
    void* MapBuffer(const AllocatedBuffer& buffer);
    void UnmapBuffer(const AllocatedBuffer& buffer);
    void FlushBuffer(const AllocatedBuffer& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
    void InvalidateBuffer(const AllocatedBuffer& buffer, VkDeviceSize offset = 0, VkDeviceSize size = VK_WHOLE_SIZE);
    
    // Data transfer utilities
    void CopyBuffer(const AllocatedBuffer& srcBuffer, 
                   const AllocatedBuffer& dstBuffer, 
                   VkDeviceSize size,
                   VkDeviceSize srcOffset = 0,
                   VkDeviceSize dstOffset = 0);
    
    void CopyBufferToImage(const AllocatedBuffer& buffer,
                          const AllocatedImage& image,
                          uint32_t width,
                          uint32_t height,
                          uint32_t layerCount = 1);
    
    // Image layout transitions
    void TransitionImageLayout(VkImage image,
                              VkFormat format,
                              VkImageLayout oldLayout,
                              VkImageLayout newLayout,
                              uint32_t mipLevels = 1,
                              uint32_t layerCount = 1);
    
    // Mipmap generation
    void GenerateMipmaps(VkImage image,
                        VkFormat format,
                        uint32_t width,
                        uint32_t height,
                        uint32_t mipLevels);
    
    // Memory statistics and debugging
    void GetMemoryUsage(VmaTotalStatistics& stats) const;
    void GetBudget(VmaBudget* budgets) const;
    size_t GetTotalAllocatedBytes() const;
    uint32_t GetAllocationCount() const;
    
    // Getters
    VmaAllocator GetAllocator() const { return m_allocator; }
    VulkanRenderer* GetRenderer() const { return m_renderer; }
    bool IsInitialized() const { return m_initialized; }

private:
    // Helper methods
    bool CreateAllocator();
    VkImageView CreateImageView(VkImage image, 
                               VkFormat format, 
                               VkImageAspectFlags aspectFlags,
                               uint32_t mipLevels = 1,
                               VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
    
    bool HasStencilComponent(VkFormat format) const;
    
    // VMA and Vulkan objects
    VmaAllocator m_allocator = VK_NULL_HANDLE;
    VulkanRenderer* m_renderer = nullptr;
    
    // State
    bool m_initialized = false;
    
    // Statistics tracking
    mutable uint32_t m_allocationCount = 0;
};