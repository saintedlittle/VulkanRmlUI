#include "VulkanCommandBuffer.h"
#include "VulkanDevice.h"
#include <iostream>
#include <stdexcept>

VulkanCommandBuffer::VulkanCommandBuffer() = default;

VulkanCommandBuffer::~VulkanCommandBuffer() {
    Cleanup();
}

bool VulkanCommandBuffer::Initialize(const InitInfo& info) {
    if (m_initialized) {
        std::cerr << "VulkanCommandBuffer already initialized" << std::endl;
        return false;
    }
    
    if (!info.device) {
        std::cerr << "VulkanDevice is null" << std::endl;
        return false;
    }
    
    m_device = info.device;
    m_queueFamilyIndex = info.queueFamilyIndex;
    m_poolFlags = info.poolFlags;
    m_initialCommandBufferCount = info.initialCommandBufferCount;
    
    try {
        if (!CreateCommandPool()) {
            std::cerr << "Failed to create command pool" << std::endl;
            return false;
        }
        
        if (!AllocateInitialCommandBuffers()) {
            std::cerr << "Failed to allocate initial command buffers" << std::endl;
            return false;
        }
        
        m_initialized = true;
        std::cout << "VulkanCommandBuffer initialized successfully with " 
                  << m_initialCommandBufferCount << " command buffers" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "VulkanCommandBuffer initialization failed: " << e.what() << std::endl;
        Cleanup();
        return false;
    }
}

void VulkanCommandBuffer::Cleanup() {
    if (!m_initialized || !m_device) {
        return;
    }
    
    VkDevice device = m_device->GetDevice();
    if (device == VK_NULL_HANDLE) {
        return;
    }
    
    // Wait for device to be idle before cleanup
    vkDeviceWaitIdle(device);
    
    // Free command buffers
    if (!m_commandBuffers.empty() && m_commandPool != VK_NULL_HANDLE) {
        vkFreeCommandBuffers(device, m_commandPool, 
                           static_cast<uint32_t>(m_commandBuffers.size()), 
                           m_commandBuffers.data());
        m_commandBuffers.clear();
    }
    
    // Destroy command pool
    if (m_commandPool != VK_NULL_HANDLE) {
        vkDestroyCommandPool(device, m_commandPool, nullptr);
        m_commandPool = VK_NULL_HANDLE;
    }
    
    m_device = nullptr;
    m_initialized = false;
}

VkCommandBuffer VulkanCommandBuffer::AllocateCommandBuffer(VkCommandBufferLevel level) {
    if (!m_initialized || !m_device) {
        throw std::runtime_error("VulkanCommandBuffer not initialized");
    }
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = 1;
    
    VkCommandBuffer commandBuffer;
    VkResult result = vkAllocateCommandBuffers(m_device->GetDevice(), &allocInfo, &commandBuffer);
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffer! Error code: " + std::to_string(result));
    }
    
    return commandBuffer;
}

std::vector<VkCommandBuffer> VulkanCommandBuffer::AllocateCommandBuffers(uint32_t count, VkCommandBufferLevel level) {
    if (!m_initialized || !m_device) {
        throw std::runtime_error("VulkanCommandBuffer not initialized");
    }
    
    if (count == 0) {
        return {};
    }
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = level;
    allocInfo.commandBufferCount = count;
    
    std::vector<VkCommandBuffer> commandBuffers(count);
    VkResult result = vkAllocateCommandBuffers(m_device->GetDevice(), &allocInfo, commandBuffers.data());
    
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate command buffers! Error code: " + std::to_string(result));
    }
    
    return commandBuffers;
}

void VulkanCommandBuffer::FreeCommandBuffer(VkCommandBuffer commandBuffer) {
    if (!m_initialized || !m_device || commandBuffer == VK_NULL_HANDLE) {
        return;
    }
    
    vkFreeCommandBuffers(m_device->GetDevice(), m_commandPool, 1, &commandBuffer);
}

void VulkanCommandBuffer::FreeCommandBuffers(const std::vector<VkCommandBuffer>& commandBuffers) {
    if (!m_initialized || !m_device || commandBuffers.empty()) {
        return;
    }
    
    vkFreeCommandBuffers(m_device->GetDevice(), m_commandPool, 
                        static_cast<uint32_t>(commandBuffers.size()), 
                        commandBuffers.data());
}

bool VulkanCommandBuffer::BeginRecording(VkCommandBuffer commandBuffer, VkCommandBufferUsageFlags usage) {
    if (!m_initialized || commandBuffer == VK_NULL_HANDLE) {
        std::cerr << "Invalid command buffer or VulkanCommandBuffer not initialized" << std::endl;
        return false;
    }
    
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = usage;
    beginInfo.pInheritanceInfo = nullptr; // Only relevant for secondary command buffers
    
    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to begin recording command buffer! Error code: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanCommandBuffer::EndRecording(VkCommandBuffer commandBuffer) {
    if (!m_initialized || commandBuffer == VK_NULL_HANDLE) {
        std::cerr << "Invalid command buffer or VulkanCommandBuffer not initialized" << std::endl;
        return false;
    }
    
    VkResult result = vkEndCommandBuffer(commandBuffer);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to end recording command buffer! Error code: " << result << std::endl;
        return false;
    }
    
    return true;
}

VkCommandBuffer VulkanCommandBuffer::BeginSingleTimeCommands() {
    if (!m_initialized || !m_device) {
        throw std::runtime_error("VulkanCommandBuffer not initialized");
    }
    
    VkCommandBuffer commandBuffer = AllocateCommandBuffer();
    
    if (!BeginRecording(commandBuffer, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT)) {
        FreeCommandBuffer(commandBuffer);
        throw std::runtime_error("Failed to begin recording single-time command buffer");
    }
    
    return commandBuffer;
}

void VulkanCommandBuffer::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
    if (!m_initialized || !m_device || commandBuffer == VK_NULL_HANDLE) {
        throw std::runtime_error("Invalid state for ending single-time commands");
    }
    
    if (!EndRecording(commandBuffer)) {
        FreeCommandBuffer(commandBuffer);
        throw std::runtime_error("Failed to end recording single-time command buffer");
    }
    
    // Submit the command buffer
    SubmitInfo submitInfo;
    submitInfo.commandBuffers = { commandBuffer };
    submitInfo.queue = m_device->GetGraphicsQueue();
    
    if (!SubmitCommandBuffers(submitInfo)) {
        FreeCommandBuffer(commandBuffer);
        throw std::runtime_error("Failed to submit single-time command buffer");
    }
    
    // Wait for completion and free the command buffer
    vkQueueWaitIdle(m_device->GetGraphicsQueue());
    FreeCommandBuffer(commandBuffer);
}

bool VulkanCommandBuffer::SubmitCommandBuffers(const SubmitInfo& submitInfo) {
    if (!m_initialized || !m_device) {
        std::cerr << "VulkanCommandBuffer not initialized" << std::endl;
        return false;
    }
    
    if (submitInfo.commandBuffers.empty()) {
        std::cerr << "No command buffers to submit" << std::endl;
        return false;
    }
    
    // Use graphics queue if no queue specified
    VkQueue queue = submitInfo.queue;
    if (queue == VK_NULL_HANDLE) {
        queue = m_device->GetGraphicsQueue();
    }
    
    if (queue == VK_NULL_HANDLE) {
        std::cerr << "No valid queue for command buffer submission" << std::endl;
        return false;
    }
    
    // Validate wait semaphores and stages match
    if (submitInfo.waitSemaphores.size() != submitInfo.waitStages.size()) {
        std::cerr << "Wait semaphores and wait stages count mismatch" << std::endl;
        return false;
    }
    
    VkSubmitInfo vkSubmitInfo{};
    vkSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    // Wait semaphores
    vkSubmitInfo.waitSemaphoreCount = static_cast<uint32_t>(submitInfo.waitSemaphores.size());
    vkSubmitInfo.pWaitSemaphores = submitInfo.waitSemaphores.empty() ? nullptr : submitInfo.waitSemaphores.data();
    vkSubmitInfo.pWaitDstStageMask = submitInfo.waitStages.empty() ? nullptr : submitInfo.waitStages.data();
    
    // Command buffers
    vkSubmitInfo.commandBufferCount = static_cast<uint32_t>(submitInfo.commandBuffers.size());
    vkSubmitInfo.pCommandBuffers = submitInfo.commandBuffers.data();
    
    // Signal semaphores
    vkSubmitInfo.signalSemaphoreCount = static_cast<uint32_t>(submitInfo.signalSemaphores.size());
    vkSubmitInfo.pSignalSemaphores = submitInfo.signalSemaphores.empty() ? nullptr : submitInfo.signalSemaphores.data();
    
    VkResult result = vkQueueSubmit(queue, 1, &vkSubmitInfo, submitInfo.fence);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to submit command buffer! Error code: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanCommandBuffer::ExecuteImmediate(std::function<void(VkCommandBuffer)> recordingFunction) {
    if (!recordingFunction) {
        std::cerr << "Recording function is null" << std::endl;
        return false;
    }
    
    try {
        VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
        recordingFunction(commandBuffer);
        EndSingleTimeCommands(commandBuffer);
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to execute immediate command: " << e.what() << std::endl;
        return false;
    }
}

bool VulkanCommandBuffer::ResetCommandPool(VkCommandPoolResetFlags flags) {
    if (!m_initialized || !m_device) {
        std::cerr << "VulkanCommandBuffer not initialized" << std::endl;
        return false;
    }
    
    VkResult result = vkResetCommandPool(m_device->GetDevice(), m_commandPool, flags);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to reset command pool! Error code: " << result << std::endl;
        return false;
    }
    
    return true;
}

VkCommandBuffer VulkanCommandBuffer::GetCommandBuffer(uint32_t index) const {
    if (!m_initialized || index >= m_commandBuffers.size()) {
        return VK_NULL_HANDLE;
    }
    
    return m_commandBuffers[index];
}

bool VulkanCommandBuffer::CreateCommandPool() {
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = m_poolFlags;
    poolInfo.queueFamilyIndex = m_queueFamilyIndex;
    
    VkResult result = vkCreateCommandPool(m_device->GetDevice(), &poolInfo, nullptr, &m_commandPool);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create command pool! Error code: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanCommandBuffer::AllocateInitialCommandBuffers() {
    if (m_initialCommandBufferCount == 0) {
        return true; // No initial command buffers requested
    }
    
    // Allocate command buffers directly without using AllocateCommandBuffers() 
    // since m_initialized is not yet set to true
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = m_initialCommandBufferCount;
    
    m_commandBuffers.resize(m_initialCommandBufferCount);
    VkResult result = vkAllocateCommandBuffers(m_device->GetDevice(), &allocInfo, m_commandBuffers.data());
    
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to allocate initial command buffers: Error code " << result << std::endl;
        m_commandBuffers.clear();
        return false;
    }
    
    return true;
}