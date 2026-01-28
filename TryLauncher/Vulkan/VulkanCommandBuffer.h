#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>
#include <functional>

class VulkanDevice;

/**
 * VulkanCommandBuffer provides a comprehensive command buffer management system
 * for the Vulkan renderer. It handles command pool creation, command buffer allocation,
 * recording utilities, and submission with proper synchronization.
 */
class VulkanCommandBuffer {
public:
    struct InitInfo {
        VulkanDevice* device = nullptr;
        uint32_t queueFamilyIndex = 0;
        VkCommandPoolCreateFlags poolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        uint32_t initialCommandBufferCount = 1;
    };
    
    VulkanCommandBuffer();
    ~VulkanCommandBuffer();
    
    // Initialization and cleanup
    bool Initialize(const InitInfo& info);
    void Cleanup();
    
    // Command pool management
    VkCommandPool GetCommandPool() const { return m_commandPool; }
    
    // Command buffer allocation
    VkCommandBuffer AllocateCommandBuffer(VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    std::vector<VkCommandBuffer> AllocateCommandBuffers(uint32_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    void FreeCommandBuffer(VkCommandBuffer commandBuffer);
    void FreeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers);
    
    // Command buffer recording utilities
    bool BeginRecording(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usage = 0);
    bool EndRecording(VkCommandBuffer commandBuffer);
    
    // Single-time command support
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    // Command buffer submission with synchronization
    struct SubmitInfo {
        std::vector<VkCommandBuffer> commandBuffers;
        std::vector<VkSemaphore> waitSemaphores;
        std::vector<VkPipelineStageFlags> waitStages;
        std::vector<VkSemaphore> signalSemaphores;
        VkFence fence = VK_NULL_HANDLE;
        VkQueue queue = VK_NULL_HANDLE; // If null, uses graphics queue
    };
    
    bool SubmitCommandBuffers(const SubmitInfo& submitInfo);
    
    // Convenience methods for common operations
    bool ExecuteImmediate(std::function<void(VkCommandBuffer)> recordingFunction);
    
    // Reset command pool (resets all command buffers allocated from this pool)
    bool ResetCommandPool(VkCommandPoolResetFlags flags = 0);
    
    // Get allocated command buffers for frame-based rendering
    const std::vector<VkCommandBuffer>& GetCommandBuffers() const { return m_commandBuffers; }
    VkCommandBuffer GetCommandBuffer(uint32_t index) const;
    
    // Statistics and debugging
    uint32_t GetAllocatedCommandBufferCount() const { return static_cast<uint32_t>(m_commandBuffers.size()); }
    bool IsInitialized() const { return m_initialized; }

private:
    // Helper methods
    bool CreateCommandPool();
    bool AllocateInitialCommandBuffers();
    
    // Vulkan objects
    VulkanDevice* m_device = nullptr;
    VkCommandPool m_commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_commandBuffers;
    
    // Configuration
    uint32_t m_queueFamilyIndex = 0;
    VkCommandPoolCreateFlags m_poolFlags = 0;
    uint32_t m_initialCommandBufferCount = 1;
    
    // State
    bool m_initialized = false;
};