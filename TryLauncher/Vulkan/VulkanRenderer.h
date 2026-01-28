#pragma once

#include "../Engine/Engine.h"
#include "../Core/EngineConfig.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandBuffer.h"
#include <vulkan/vulkan.h>
#include <memory>

struct GLFWwindow;
class SettingsManager;

class VulkanRenderer : public IEngineModule {
public:
    struct InitInfo {
        GLFWwindow* window = nullptr;
        bool enableValidation = false;
        std::vector<const char*> requiredExtensions;
    };
    
    VulkanRenderer(SettingsManager* settingsManager);
    ~VulkanRenderer();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "VulkanRenderer"; }
    int GetInitializationOrder() const override { return 300; }

    // Vulkan-specific methods
    void BeginFrame();
    void EndFrame();
    void WaitIdle();
    
    // Resource creation
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
    
    // Getters for integration
    VkDevice GetDevice() const;
    VkPhysicalDevice GetPhysicalDevice() const;
    VkQueue GetGraphicsQueue() const;
    VkQueue GetPresentQueue() const;
    VkQueue GetTransferQueue() const;
    VkCommandPool GetCommandPool() const;
    VulkanDevice* GetVulkanDevice() const { return m_device.get(); }
    VulkanSwapchain* GetSwapchain() const { return m_swapchain.get(); }
    VulkanCommandBuffer* GetCommandBuffer() const { return m_commandBuffer.get(); }
    
    // Window resize handling
    void OnWindowResize();
    
    // Settings application
    void ApplyGraphicsSettings(const EngineConfig::Graphics& graphics);
    void OnSettingsChanged(const std::string& settingName);

private:
    bool CreateCommandBuffers();
    bool CreateSwapchain();
    
    SettingsManager* m_settingsManager;
    std::unique_ptr<VulkanDevice> m_device;
    std::unique_ptr<VulkanSwapchain> m_swapchain;
    std::unique_ptr<VulkanCommandBuffer> m_commandBuffer;
    GLFWwindow* m_window = nullptr;
    
    // Frame state
    uint32_t m_currentImageIndex = 0;
    bool m_framebufferResized = false;
    
    bool m_initialized = false;
};