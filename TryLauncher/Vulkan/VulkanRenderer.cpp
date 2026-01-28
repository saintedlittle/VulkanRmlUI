#include "VulkanRenderer.h"
#include "VulkanDevice.h"
#include "VulkanSwapchain.h"
#include "VulkanCommandBuffer.h"
#include "../Core/SettingsManager.h"
#include "../Core/EngineConfig.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdexcept>

VulkanRenderer::VulkanRenderer(SettingsManager* settingsManager)
    : m_settingsManager(settingsManager) {
}

VulkanRenderer::~VulkanRenderer() {
    if (m_initialized) {
        Shutdown();
    }
}

bool VulkanRenderer::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing VulkanRenderer..." << std::endl;
    
    try {
        // Initialize GLFW
        if (!glfwInit()) {
            std::cerr << "Failed to initialize GLFW" << std::endl;
            return false;
        }
        
        // Configure GLFW for Vulkan
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE); // Enable resizing with swapchain recreation
        
        // Get window settings from SettingsManager
        // For now, use default values - will be integrated with SettingsManager in later tasks
        uint32_t width = 1920;
        uint32_t height = 1080;
        bool fullscreen = false;
        bool enableValidation = false; // Will be configurable later
        
        // Create window
        GLFWmonitor* monitor = fullscreen ? glfwGetPrimaryMonitor() : nullptr;
        m_window = glfwCreateWindow(width, height, "Vulkan RmlUI Game Engine", monitor, nullptr);
        
        if (!m_window) {
            std::cerr << "Failed to create GLFW window" << std::endl;
            glfwTerminate();
            return false;
        }
        
        // Initialize Vulkan device
        m_device = std::make_unique<VulkanDevice>();
        
        VulkanDevice::InitInfo deviceInfo;
        deviceInfo.window = m_window;
        deviceInfo.enableValidation = enableValidation;
        // Required extensions will be automatically determined by VulkanDevice
        
        if (!m_device->Initialize(deviceInfo)) {
            std::cerr << "Failed to initialize Vulkan device" << std::endl;
            return false;
        }
        
        // Create command buffer management system
        if (!CreateCommandBuffers()) {
            std::cerr << "Failed to create command buffer management system" << std::endl;
            return false;
        }
        
        // Create swapchain
        if (!CreateSwapchain()) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
        }
        
        // Set up window resize callback
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
            auto renderer = reinterpret_cast<VulkanRenderer*>(glfwGetWindowUserPointer(window));
            renderer->m_framebufferResized = true;
        });
        
        m_initialized = true;
        std::cout << "VulkanRenderer initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "VulkanRenderer initialization failed: " << e.what() << std::endl;
        Shutdown();
        return false;
    }
}

void VulkanRenderer::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }
    
    // Check if window should close
    if (glfwWindowShouldClose(m_window)) {
        // Request engine shutdown - will be integrated with Engine class later
        return;
    }
    
    // Poll events
    glfwPollEvents();
    
    // TODO: Implement rendering loop in later tasks
}

void VulkanRenderer::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down VulkanRenderer..." << std::endl;
    
    // Wait for device to be idle
    if (m_device && m_device->GetDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device->GetDevice());
    }
    
    // Cleanup command buffer management system
    if (m_commandBuffer) {
        m_commandBuffer->Cleanup();
        m_commandBuffer.reset();
    }
    
    // Cleanup swapchain
    if (m_swapchain) {
        m_swapchain->Cleanup();
        m_swapchain.reset();
    }
    
    // Cleanup Vulkan device
    if (m_device) {
        m_device->Cleanup();
        m_device.reset();
    }
    
    // Cleanup GLFW
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
    
    m_initialized = false;
    std::cout << "VulkanRenderer shutdown complete" << std::endl;
}

void VulkanRenderer::BeginFrame() {
    if (!m_initialized || !m_swapchain) {
        return;
    }
    
    // Handle swapchain recreation if needed
    if (m_swapchain->IsOutOfDate() || m_framebufferResized) {
        m_framebufferResized = false;
        if (!m_swapchain->RecreateSwapchain()) {
            std::cerr << "Failed to recreate swapchain" << std::endl;
            return;
        }
    }
    
    // Acquire next image from swapchain
    if (!m_swapchain->AcquireNextImage(m_currentImageIndex)) {
        // Swapchain is out of date, will be recreated on next frame
        return;
    }
}

void VulkanRenderer::EndFrame() {
    if (!m_initialized || !m_swapchain) {
        return;
    }
    
    // Present the image
    if (!m_swapchain->PresentImage(m_currentImageIndex)) {
        // Swapchain is out of date, will be recreated on next frame
        m_swapchain->MarkOutOfDate();
    }
    
    // Advance to next frame
    m_swapchain->AdvanceFrame();
}

void VulkanRenderer::WaitIdle() {
    if (m_device && m_device->GetDevice() != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_device->GetDevice());
    }
}

VkCommandBuffer VulkanRenderer::BeginSingleTimeCommands() {
    if (!m_commandBuffer) {
        throw std::runtime_error("VulkanCommandBuffer not initialized");
    }
    
    return m_commandBuffer->BeginSingleTimeCommands();
}

void VulkanRenderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
    if (!m_commandBuffer) {
        throw std::runtime_error("VulkanCommandBuffer not initialized");
    }
    
    m_commandBuffer->EndSingleTimeCommands(commandBuffer);
}

void VulkanRenderer::OnWindowResize() {
    m_framebufferResized = true;
}
VkDevice VulkanRenderer::GetDevice() const {
    return m_device ? m_device->GetDevice() : VK_NULL_HANDLE;
}

VkPhysicalDevice VulkanRenderer::GetPhysicalDevice() const {
    return m_device ? m_device->GetPhysicalDevice() : VK_NULL_HANDLE;
}

VkQueue VulkanRenderer::GetGraphicsQueue() const {
    return m_device ? m_device->GetGraphicsQueue() : VK_NULL_HANDLE;
}

VkQueue VulkanRenderer::GetPresentQueue() const {
    return m_device ? m_device->GetPresentQueue() : VK_NULL_HANDLE;
}

VkQueue VulkanRenderer::GetTransferQueue() const {
    return m_device ? m_device->GetTransferQueue() : VK_NULL_HANDLE;
}

VkCommandPool VulkanRenderer::GetCommandPool() const {
    return m_commandBuffer ? m_commandBuffer->GetCommandPool() : VK_NULL_HANDLE;
}

bool VulkanRenderer::CreateCommandBuffers() {
    // Create the VulkanCommandBuffer management system
    m_commandBuffer = std::make_unique<VulkanCommandBuffer>();
    
    const QueueFamilyIndices& queueFamilyIndices = m_device->GetQueueFamilyIndices();
    
    VulkanCommandBuffer::InitInfo commandBufferInfo;
    commandBufferInfo.device = m_device.get();
    commandBufferInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    commandBufferInfo.poolFlags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    commandBufferInfo.initialCommandBufferCount = 1; // Start with one command buffer
    
    if (!m_commandBuffer->Initialize(commandBufferInfo)) {
        std::cerr << "Failed to initialize VulkanCommandBuffer" << std::endl;
        m_commandBuffer.reset();
        return false;
    }
    
    return true;
}

bool VulkanRenderer::CreateSwapchain() {
    m_swapchain = std::make_unique<VulkanSwapchain>();
    
    VulkanSwapchain::InitInfo swapchainInfo;
    swapchainInfo.device = m_device.get();
    swapchainInfo.window = m_window;
    swapchainInfo.preferredWidth = 0; // Use window size
    swapchainInfo.preferredHeight = 0; // Use window size
    swapchainInfo.enableVSync = true; // Will be configurable later via SettingsManager
    
    if (!m_swapchain->Initialize(swapchainInfo)) {
        std::cerr << "Failed to initialize swapchain" << std::endl;
        m_swapchain.reset();
        return false;
    }
    
    return true;
}

void VulkanRenderer::ApplyGraphicsSettings(const EngineConfig::Graphics& graphics) {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Applying graphics settings..." << std::endl;
    
    // Wait for device to be idle before making changes
    WaitIdle();
    
    // Apply window size changes
    if (m_window) {
        int currentWidth, currentHeight;
        glfwGetWindowSize(m_window, &currentWidth, &currentHeight);
        
        if (static_cast<uint32_t>(currentWidth) != graphics.windowWidth || 
            static_cast<uint32_t>(currentHeight) != graphics.windowHeight) {
            glfwSetWindowSize(m_window, graphics.windowWidth, graphics.windowHeight);
            OnWindowResize();
        }
        
        // Apply fullscreen mode
        GLFWmonitor* currentMonitor = glfwGetWindowMonitor(m_window);
        bool isCurrentlyFullscreen = (currentMonitor != nullptr);
        
        if (graphics.fullscreen != isCurrentlyFullscreen) {
            if (graphics.fullscreen) {
                // Switch to fullscreen
                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                const GLFWvidmode* mode = glfwGetVideoMode(monitor);
                glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            } else {
                // Switch to windowed mode
                glfwSetWindowMonitor(m_window, nullptr, 100, 100, graphics.windowWidth, graphics.windowHeight, 0);
            }
            OnWindowResize();
        }
    }
    
    // Apply VSync setting
    if (m_swapchain) {
        // VSync changes require swapchain recreation
        // This will be handled during the next frame when swapchain is recreated
        m_framebufferResized = true;
    }
    
    // Note: MSAA and validation settings require more complex changes
    // and would typically require full renderer reinitialization
    // For now, we'll log that these settings require restart
    if (graphics.msaaSamples != 1) {
        std::cout << "MSAA setting will take effect on next application restart" << std::endl;
    }
    
    if (graphics.enableValidation) {
        std::cout << "Validation layers setting will take effect on next application restart" << std::endl;
    }
    
    std::cout << "Graphics settings applied successfully" << std::endl;
}

void VulkanRenderer::OnSettingsChanged(const std::string& settingName) {
    if (!m_initialized || !m_settingsManager) {
        return;
    }
    
    // Handle graphics-related setting changes
    if (settingName.find("graphics.") == 0) {
        const auto& config = m_settingsManager->GetConfig();
        ApplyGraphicsSettings(config.graphics);
    }
}