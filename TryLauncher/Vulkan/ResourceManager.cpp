#define VMA_IMPLEMENTATION
#include "ResourceManager.h"
#include "VulkanDevice.h"
#include "VulkanRenderer.h"
#include <stdexcept>
#include <iostream>

ResourceManager::ResourceManager(VulkanRenderer* renderer) : m_renderer(renderer) {
}

ResourceManager::~ResourceManager() {
    if (m_initialized) {
        Shutdown();
    }
}

bool ResourceManager::Initialize() {
    if (m_initialized) {
        std::cerr << "ResourceManager: Already initialized" << std::endl;
        return false;
    }
    
    if (!m_renderer) {
        std::cerr << "ResourceManager: Invalid renderer parameter" << std::endl;
        return false;
    }
    
    if (!CreateAllocator()) {
        std::cerr << "ResourceManager: Failed to create VMA allocator" << std::endl;
        return false;
    }
    
    m_initialized = true;
    std::cout << "ResourceManager: Successfully initialized with VMA" << std::endl;
    return true;
}

void ResourceManager::Update(float deltaTime) {
    // ResourceManager doesn't need per-frame updates
}

void ResourceManager::Shutdown() {
    Cleanup();
}

void ResourceManager::Cleanup() {
    if (!m_initialized) {
        return;
    }
    
    if (m_allocator != VK_NULL_HANDLE) {
        // Print memory statistics before cleanup
        VmaTotalStatistics stats;
        vmaCalculateStatistics(m_allocator, &stats);
        
        if (stats.total.statistics.allocationBytes > 0) {
            std::cout << "ResourceManager: Warning - " << stats.total.statistics.allocationCount 
                     << " allocations still active (" << stats.total.statistics.allocationBytes << " bytes)" << std::endl;
        }
        
        vmaDestroyAllocator(m_allocator);
        m_allocator = VK_NULL_HANDLE;
    }
    
    m_renderer = nullptr;
    m_initialized = false;
    m_allocationCount = 0;
    
    std::cout << "ResourceManager: Cleanup completed" << std::endl;
}

bool ResourceManager::CreateAllocator() {
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_0;
    allocatorInfo.physicalDevice = m_renderer->GetPhysicalDevice();
    allocatorInfo.device = m_renderer->GetDevice();
    allocatorInfo.instance = m_renderer->GetVulkanDevice()->GetInstance();
    
    VkResult result = vmaCreateAllocator(&allocatorInfo, &m_allocator);
    if (result != VK_SUCCESS) {
        std::cerr << "ResourceManager: Failed to create VMA allocator: " << result << std::endl;
        return false;
    }
    
    std::cout << "ResourceManager: VMA allocator created successfully" << std::endl;
    return true;
}

AllocatedBuffer ResourceManager::CreateBuffer(VkDeviceSize size, 
                                             VkBufferUsageFlags usage, 
                                             VmaMemoryUsage memoryUsage,
                                             VmaAllocationCreateFlags flags) {
    if (!m_initialized) {
        std::cerr << "ResourceManager: Not initialized" << std::endl;
        return {};
    }
    
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;
    
    AllocatedBuffer buffer = {};
    VkResult result = vmaCreateBuffer(m_allocator, &bufferInfo, &allocInfo, 
                                     &buffer.buffer, &buffer.allocation, &buffer.info);
    
    if (result != VK_SUCCESS) {
        std::cerr << "ResourceManager: Failed to create buffer: " << result << std::endl;
        return {};
    }
    
    m_allocationCount++;
    return buffer;
}

AllocatedBuffer ResourceManager::CreateVertexBuffer(VkDeviceSize size, bool hostVisible) {
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VmaMemoryUsage memoryUsage;
    VmaAllocationCreateFlags flags = 0;
    
    if (hostVisible) {
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else {
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    }
    
    return CreateBuffer(size, usage, memoryUsage, flags);
}

AllocatedBuffer ResourceManager::CreateIndexBuffer(VkDeviceSize size, bool hostVisible) {
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    VmaMemoryUsage memoryUsage;
    VmaAllocationCreateFlags flags = 0;
    
    if (hostVisible) {
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        memoryUsage = VMA_MEMORY_USAGE_CPU_TO_GPU;
        flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;
    } else {
        usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        memoryUsage = VMA_MEMORY_USAGE_GPU_ONLY;
    }
    
    return CreateBuffer(size, usage, memoryUsage, flags);
}

AllocatedBuffer ResourceManager::CreateUniformBuffer(VkDeviceSize size) {
    return CreateBuffer(size, 
                       VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                       VMA_MEMORY_USAGE_CPU_TO_GPU,
                       VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

AllocatedBuffer ResourceManager::CreateStagingBuffer(VkDeviceSize size) {
    return CreateBuffer(size,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VMA_MEMORY_USAGE_CPU_ONLY,
                       VMA_ALLOCATION_CREATE_MAPPED_BIT);
}

AllocatedImage ResourceManager::CreateImage(const VkImageCreateInfo& imageInfo, 
                                           VmaMemoryUsage memoryUsage,
                                           VmaAllocationCreateFlags flags) {
    if (!m_initialized) {
        std::cerr << "ResourceManager: Not initialized" << std::endl;
        return {};
    }
    
    VmaAllocationCreateInfo allocInfo = {};
    allocInfo.usage = memoryUsage;
    allocInfo.flags = flags;
    
    AllocatedImage image = {};
    VkResult result = vmaCreateImage(m_allocator, &imageInfo, &allocInfo,
                                    &image.image, &image.allocation, &image.info);
    
    if (result != VK_SUCCESS) {
        std::cerr << "ResourceManager: Failed to create image: " << result << std::endl;
        return {};
    }
    
    // Store image properties
    image.format = imageInfo.format;
    image.extent = imageInfo.extent;
    image.mipLevels = imageInfo.mipLevels;
    
    // Create image view
    VkImageAspectFlags aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
    if (imageInfo.format == VK_FORMAT_D32_SFLOAT || 
        imageInfo.format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
        imageInfo.format == VK_FORMAT_D24_UNORM_S8_UINT) {
        aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (HasStencilComponent(imageInfo.format)) {
            aspectFlags |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    
    image.imageView = CreateImageView(image.image, imageInfo.format, aspectFlags, imageInfo.mipLevels);
    if (image.imageView == VK_NULL_HANDLE) {
        std::cerr << "ResourceManager: Failed to create image view" << std::endl;
        vmaDestroyImage(m_allocator, image.image, image.allocation);
        return {};
    }
    
    m_allocationCount++;
    return image;
}

AllocatedImage ResourceManager::CreateImage2D(uint32_t width, 
                                             uint32_t height, 
                                             VkFormat format,
                                             VkImageUsageFlags usage,
                                             uint32_t mipLevels,
                                             VkSampleCountFlagBits samples) {
    VkImageCreateInfo imageInfo = {};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = samples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    return CreateImage(imageInfo, VMA_MEMORY_USAGE_GPU_ONLY);
}

AllocatedImage ResourceManager::CreateTexture2D(uint32_t width,
                                               uint32_t height,
                                               VkFormat format,
                                               uint32_t mipLevels) {
    VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | 
                             VK_IMAGE_USAGE_SAMPLED_BIT;
    
    if (mipLevels > 1) {
        usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }
    
    return CreateImage2D(width, height, format, usage, mipLevels);
}

void ResourceManager::DestroyBuffer(const AllocatedBuffer& buffer) {
    if (!m_initialized || !buffer.IsValid()) {
        return;
    }
    
    vmaDestroyBuffer(m_allocator, buffer.buffer, buffer.allocation);
    if (m_allocationCount > 0) {
        m_allocationCount--;
    }
}

void ResourceManager::DestroyImage(const AllocatedImage& image) {
    if (!m_initialized || !image.IsValid()) {
        return;
    }
    
    if (image.imageView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_renderer->GetDevice(), image.imageView, nullptr);
    }
    
    vmaDestroyImage(m_allocator, image.image, image.allocation);
    if (m_allocationCount > 0) {
        m_allocationCount--;
    }
}

void* ResourceManager::MapBuffer(const AllocatedBuffer& buffer) {
    if (!m_initialized || !buffer.IsValid()) {
        return nullptr;
    }
    
    // Check if buffer is already mapped
    if (buffer.info.pMappedData != nullptr) {
        return buffer.info.pMappedData;
    }
    
    void* data;
    VkResult result = vmaMapMemory(m_allocator, buffer.allocation, &data);
    if (result != VK_SUCCESS) {
        std::cerr << "ResourceManager: Failed to map buffer memory: " << result << std::endl;
        return nullptr;
    }
    
    return data;
}

void ResourceManager::UnmapBuffer(const AllocatedBuffer& buffer) {
    if (!m_initialized || !buffer.IsValid()) {
        return;
    }
    
    vmaUnmapMemory(m_allocator, buffer.allocation);
}

void ResourceManager::FlushBuffer(const AllocatedBuffer& buffer, VkDeviceSize offset, VkDeviceSize size) {
    if (!m_initialized || !buffer.IsValid()) {
        return;
    }
    
    vmaFlushAllocation(m_allocator, buffer.allocation, offset, size);
}

void ResourceManager::InvalidateBuffer(const AllocatedBuffer& buffer, VkDeviceSize offset, VkDeviceSize size) {
    if (!m_initialized || !buffer.IsValid()) {
        return;
    }
    
    vmaInvalidateAllocation(m_allocator, buffer.allocation, offset, size);
}

void ResourceManager::CopyBuffer(const AllocatedBuffer& srcBuffer, 
                                const AllocatedBuffer& dstBuffer, 
                                VkDeviceSize size,
                                VkDeviceSize srcOffset,
                                VkDeviceSize dstOffset) {
    if (!m_initialized || !srcBuffer.IsValid() || !dstBuffer.IsValid()) {
        return;
    }
    
    VkCommandBuffer commandBuffer = m_renderer->BeginSingleTimeCommands();
    
    VkBufferCopy copyRegion = {};
    copyRegion.srcOffset = srcOffset;
    copyRegion.dstOffset = dstOffset;
    copyRegion.size = size;
    
    vkCmdCopyBuffer(commandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &copyRegion);
    
    m_renderer->EndSingleTimeCommands(commandBuffer);
}

void ResourceManager::CopyBufferToImage(const AllocatedBuffer& buffer,
                                       const AllocatedImage& image,
                                       uint32_t width,
                                       uint32_t height,
                                       uint32_t layerCount) {
    if (!m_initialized || !buffer.IsValid() || !image.IsValid()) {
        return;
    }
    
    VkCommandBuffer commandBuffer = m_renderer->BeginSingleTimeCommands();
    
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = layerCount;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};
    
    vkCmdCopyBufferToImage(commandBuffer, buffer.buffer, image.image, 
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    m_renderer->EndSingleTimeCommands(commandBuffer);
}

void ResourceManager::TransitionImageLayout(VkImage image,
                                           VkFormat format,
                                           VkImageLayout oldLayout,
                                           VkImageLayout newLayout,
                                           uint32_t mipLevels,
                                           uint32_t layerCount) {
    if (!m_initialized) {
        return;
    }
    
    VkCommandBuffer commandBuffer = m_renderer->BeginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = layerCount;
    
    // Handle depth/stencil formats
    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (HasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    
    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;
    
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        std::cerr << "ResourceManager: Unsupported layout transition" << std::endl;
        return;
    }
    
    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
    
    m_renderer->EndSingleTimeCommands(commandBuffer);
}

void ResourceManager::GenerateMipmaps(VkImage image,
                                     VkFormat format,
                                     uint32_t width,
                                     uint32_t height,
                                     uint32_t mipLevels) {
    if (!m_initialized) {
        return;
    }
    
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_renderer->GetPhysicalDevice(), format, &formatProperties);
    
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        std::cerr << "ResourceManager: Texture image format does not support linear blitting" << std::endl;
        return;
    }
    
    VkCommandBuffer commandBuffer = m_renderer->BeginSingleTimeCommands();
    
    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;
    
    int32_t mipWidth = static_cast<int32_t>(width);
    int32_t mipHeight = static_cast<int32_t>(height);
    
    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        
        vkCmdPipelineBarrier(commandBuffer,
                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);
        
        VkImageBlit blit = {};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;
        
        vkCmdBlitImage(commandBuffer,
                      image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      1, &blit,
                      VK_FILTER_LINEAR);
        
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        
        vkCmdPipelineBarrier(commandBuffer,
                            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                            0, nullptr,
                            0, nullptr,
                            1, &barrier);
        
        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }
    
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
    vkCmdPipelineBarrier(commandBuffer,
                        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                        0, nullptr,
                        0, nullptr,
                        1, &barrier);
    
    m_renderer->EndSingleTimeCommands(commandBuffer);
}

VkImageView ResourceManager::CreateImageView(VkImage image, 
                                            VkFormat format, 
                                            VkImageAspectFlags aspectFlags,
                                            uint32_t mipLevels,
                                            VkImageViewType viewType) {
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = viewType;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = mipLevels;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VkImageView imageView;
    VkResult result = vkCreateImageView(m_renderer->GetDevice(), &viewInfo, nullptr, &imageView);
    if (result != VK_SUCCESS) {
        std::cerr << "ResourceManager: Failed to create image view: " << result << std::endl;
        return VK_NULL_HANDLE;
    }
    
    return imageView;
}

bool ResourceManager::HasStencilComponent(VkFormat format) const {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void ResourceManager::GetMemoryUsage(VmaTotalStatistics& stats) const {
    if (!m_initialized) {
        return;
    }
    
    vmaCalculateStatistics(m_allocator, &stats);
}

void ResourceManager::GetBudget(VmaBudget* budgets) const {
    if (!m_initialized) {
        return;
    }
    
    vmaGetHeapBudgets(m_allocator, budgets);
}

size_t ResourceManager::GetTotalAllocatedBytes() const {
    if (!m_initialized) {
        return 0;
    }
    
    VmaTotalStatistics stats;
    vmaCalculateStatistics(m_allocator, &stats);
    return stats.total.statistics.allocationBytes;
}

uint32_t ResourceManager::GetAllocationCount() const {
    return m_allocationCount;
}