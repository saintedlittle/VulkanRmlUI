#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <memory>

class VulkanDevice;
struct GLFWwindow;

class VulkanSwapchain {
public:
    struct InitInfo {
        VulkanDevice* device = nullptr;
        GLFWwindow* window = nullptr;
        uint32_t preferredWidth = 0;
        uint32_t preferredHeight = 0;
        bool enableVSync = true;
    };
    
    VulkanSwapchain();
    ~VulkanSwapchain();
    
    // Initialization and cleanup
    bool Initialize(const InitInfo& info);
    void Cleanup();
    
    // Frame operations
    bool AcquireNextImage(uint32_t& imageIndex);
    bool PresentImage(uint32_t imageIndex);
    
    // Swapchain recreation (for window resize)
    bool RecreateSwapchain();
    bool IsOutOfDate() const { return m_outOfDate; }
    void MarkOutOfDate() { m_outOfDate = true; }
    
    // Getters
    VkSwapchainKHR GetSwapchain() const { return m_swapchain; }
    VkFormat GetImageFormat() const { return m_swapchainImageFormat; }
    VkExtent2D GetExtent() const { return m_swapchainExtent; }
    const std::vector<VkImage>& GetImages() const { return m_swapchainImages; }
    const std::vector<VkImageView>& GetImageViews() const { return m_swapchainImageViews; }
    uint32_t GetImageCount() const { return static_cast<uint32_t>(m_swapchainImages.size()); }
    
    // Synchronization objects
    VkSemaphore GetImageAvailableSemaphore() const { return m_imageAvailableSemaphores[m_currentFrame]; }
    VkSemaphore GetRenderFinishedSemaphore() const { return m_renderFinishedSemaphores[m_currentFrame]; }
    VkFence GetInFlightFence() const { return m_inFlightFences[m_currentFrame]; }
    
    // Frame management
    void AdvanceFrame() { m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; }
    uint32_t GetCurrentFrame() const { return m_currentFrame; }
    
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;

private:
    // Swapchain creation helpers
    bool CreateSwapchain();
    bool CreateImageViews();
    bool CreateSyncObjects();
    
    // Swapchain configuration helpers
    VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    // Cleanup helpers
    void CleanupSwapchain();
    void CleanupSyncObjects();
    
    // Configuration
    InitInfo m_initInfo;
    
    // Vulkan objects
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    VkFormat m_swapchainImageFormat;
    VkExtent2D m_swapchainExtent;
    
    // Synchronization objects
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    
    // Frame management
    uint32_t m_currentFrame = 0;
    bool m_outOfDate = false;
    
    // References (not owned)
    VulkanDevice* m_device = nullptr;
    GLFWwindow* m_window = nullptr;
};