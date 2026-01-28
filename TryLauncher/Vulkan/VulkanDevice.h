#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <optional>

struct GLFWwindow;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;
    std::optional<uint32_t> transferFamily;
    
    bool IsComplete() const {
        return graphicsFamily.has_value() && 
               presentFamily.has_value() && 
               transferFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanDevice {
public:
    struct InitInfo {
        GLFWwindow* window = nullptr;
        bool enableValidation = false;
        std::vector<const char*> requiredExtensions;
        std::vector<const char*> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };
    
    VulkanDevice();
    ~VulkanDevice();
    
    // Initialization and cleanup
    bool Initialize(const InitInfo& info);
    void Cleanup();
    
    // Getters for Vulkan objects
    VkInstance GetInstance() const { return m_instance; }
    VkPhysicalDevice GetPhysicalDevice() const { return m_physicalDevice; }
    VkDevice GetDevice() const { return m_device; }
    VkSurfaceKHR GetSurface() const { return m_surface; }
    
    // Queue access
    VkQueue GetGraphicsQueue() const { return m_graphicsQueue; }
    VkQueue GetPresentQueue() const { return m_presentQueue; }
    VkQueue GetTransferQueue() const { return m_transferQueue; }
    
    // Queue family indices
    const QueueFamilyIndices& GetQueueFamilyIndices() const { return m_queueFamilyIndices; }
    
    // Device properties and features
    const VkPhysicalDeviceProperties& GetDeviceProperties() const { return m_deviceProperties; }
    const VkPhysicalDeviceFeatures& GetDeviceFeatures() const { return m_deviceFeatures; }
    
    // Swapchain support
    SwapChainSupportDetails QuerySwapChainSupport() const;
    SwapChainSupportDetails QuerySwapChainSupportForDevice(VkPhysicalDevice device) const;
    
    // Utility functions
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;
    VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, 
                                VkImageTiling tiling, 
                                VkFormatFeatureFlags features) const;
    
    // Command buffer utilities
    VkCommandBuffer BeginSingleTimeCommands(VkCommandPool commandPool) const;
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer, VkCommandPool commandPool) const;

private:
    // Initialization steps
    bool CreateInstance(const InitInfo& info);
    bool SetupDebugMessenger();
    bool CreateSurface(GLFWwindow* window);
    bool PickPhysicalDevice();
    bool CreateLogicalDevice();
    
    // Helper functions
    bool CheckValidationLayerSupport() const;
    std::vector<const char*> GetRequiredExtensions(const InitInfo& info) const;
    bool IsDeviceSuitable(VkPhysicalDevice device) const;
    QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
    bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
    int RateDeviceSuitability(VkPhysicalDevice device) const;
    
    // Debug callback
    static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData);
    
    // Vulkan objects
    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkDevice m_device = VK_NULL_HANDLE;
    
    // Queues
    VkQueue m_graphicsQueue = VK_NULL_HANDLE;
    VkQueue m_presentQueue = VK_NULL_HANDLE;
    VkQueue m_transferQueue = VK_NULL_HANDLE;
    
    // Device info
    QueueFamilyIndices m_queueFamilyIndices;
    VkPhysicalDeviceProperties m_deviceProperties;
    VkPhysicalDeviceFeatures m_deviceFeatures;
    
    // Configuration
    bool m_enableValidation = false;
    std::vector<const char*> m_deviceExtensions;
    
    // Validation layers
    const std::vector<const char*> m_validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
};