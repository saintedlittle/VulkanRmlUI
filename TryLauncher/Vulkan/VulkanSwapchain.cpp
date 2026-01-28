#include "VulkanSwapchain.h"
#include "VulkanDevice.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <algorithm>
#include <limits>
#include <stdexcept>

VulkanSwapchain::VulkanSwapchain() = default;

VulkanSwapchain::~VulkanSwapchain() {
    Cleanup();
}

bool VulkanSwapchain::Initialize(const InitInfo& info) {
    if (!info.device || !info.window) {
        std::cerr << "VulkanSwapchain: Invalid initialization info" << std::endl;
        return false;
    }
    
    m_initInfo = info;
    m_device = info.device;
    m_window = info.window;
    
    try {
        if (!CreateSwapchain()) {
            std::cerr << "Failed to create swapchain" << std::endl;
            return false;
        }
        
        if (!CreateImageViews()) {
            std::cerr << "Failed to create image views" << std::endl;
            return false;
        }
        
        if (!CreateSyncObjects()) {
            std::cerr << "Failed to create synchronization objects" << std::endl;
            return false;
        }
        
        std::cout << "VulkanSwapchain initialized successfully" << std::endl;
        std::cout << "  Format: " << m_swapchainImageFormat << std::endl;
        std::cout << "  Extent: " << m_swapchainExtent.width << "x" << m_swapchainExtent.height << std::endl;
        std::cout << "  Images: " << m_swapchainImages.size() << std::endl;
        
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "VulkanSwapchain initialization failed: " << e.what() << std::endl;
        Cleanup();
        return false;
    }
}

void VulkanSwapchain::Cleanup() {
    if (!m_device || m_device->GetDevice() == VK_NULL_HANDLE) {
        return;
    }
    
    // Wait for device to be idle before cleanup
    vkDeviceWaitIdle(m_device->GetDevice());
    
    CleanupSyncObjects();
    CleanupSwapchain();
}

bool VulkanSwapchain::AcquireNextImage(uint32_t& imageIndex) {
    if (!m_device || m_swapchain == VK_NULL_HANDLE) {
        return false;
    }
    
    // Wait for the current frame's fence
    vkWaitForFences(m_device->GetDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);
    
    VkResult result = vkAcquireNextImageKHR(
        m_device->GetDevice(),
        m_swapchain,
        UINT64_MAX,
        m_imageAvailableSemaphores[m_currentFrame],
        VK_NULL_HANDLE,
        &imageIndex
    );
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        m_outOfDate = true;
        return false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        std::cerr << "Failed to acquire swapchain image! Error code: " << result << std::endl;
        return false;
    }
    
    // Reset the fence only if we're submitting work
    vkResetFences(m_device->GetDevice(), 1, &m_inFlightFences[m_currentFrame]);
    
    return true;
}

bool VulkanSwapchain::PresentImage(uint32_t imageIndex) {
    if (!m_device || m_swapchain == VK_NULL_HANDLE) {
        return false;
    }
    
    VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[m_currentFrame] };
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapchains[] = { m_swapchain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapchains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optional
    
    VkResult result = vkQueuePresentKHR(m_device->GetPresentQueue(), &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        m_outOfDate = true;
        return false;
    } else if (result != VK_SUCCESS) {
        std::cerr << "Failed to present swapchain image! Error code: " << result << std::endl;
        return false;
    }
    
    return true;
}

bool VulkanSwapchain::RecreateSwapchain() {
    if (!m_device || !m_window) {
        return false;
    }
    
    // Handle window minimization
    int width = 0, height = 0;
    glfwGetFramebufferSize(m_window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(m_window, &width, &height);
        glfwWaitEvents();
    }
    
    // Wait for device to be idle
    vkDeviceWaitIdle(m_device->GetDevice());
    
    // Cleanup old swapchain
    CleanupSwapchain();
    
    // Create new swapchain
    if (!CreateSwapchain()) {
        std::cerr << "Failed to recreate swapchain" << std::endl;
        return false;
    }
    
    if (!CreateImageViews()) {
        std::cerr << "Failed to recreate image views" << std::endl;
        return false;
    }
    
    m_outOfDate = false;
    
    std::cout << "Swapchain recreated successfully" << std::endl;
    std::cout << "  New extent: " << m_swapchainExtent.width << "x" << m_swapchainExtent.height << std::endl;
    
    return true;
}

bool VulkanSwapchain::CreateSwapchain() {
    SwapChainSupportDetails swapChainSupport = m_device->QuerySwapChainSupport();
    
    VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities);
    
    // Request one more image than the minimum to avoid waiting on the driver
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && 
        imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }
    
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = m_device->GetSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    
    QueueFamilyIndices indices = m_device->GetQueueFamilyIndices();
    uint32_t queueFamilyIndices[] = {
        indices.graphicsFamily.value(),
        indices.presentFamily.value()
    };
    
    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0; // Optional
        createInfo.pQueueFamilyIndices = nullptr; // Optional
    }
    
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE; // For swapchain recreation
    
    VkResult result = vkCreateSwapchainKHR(m_device->GetDevice(), &createInfo, nullptr, &m_swapchain);
    if (result != VK_SUCCESS) {
        std::cerr << "Failed to create swapchain! Error code: " << result << std::endl;
        return false;
    }
    
    // Get swapchain images
    vkGetSwapchainImagesKHR(m_device->GetDevice(), m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_device->GetDevice(), m_swapchain, &imageCount, m_swapchainImages.data());
    
    // Store format and extent
    m_swapchainImageFormat = surfaceFormat.format;
    m_swapchainExtent = extent;
    
    return true;
}

bool VulkanSwapchain::CreateImageViews() {
    m_swapchainImageViews.resize(m_swapchainImages.size());
    
    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = m_swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = m_swapchainImageFormat;
        
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        VkResult result = vkCreateImageView(m_device->GetDevice(), &createInfo, nullptr, &m_swapchainImageViews[i]);
        if (result != VK_SUCCESS) {
            std::cerr << "Failed to create image view " << i << "! Error code: " << result << std::endl;
            return false;
        }
    }
    
    return true;
}

bool VulkanSwapchain::CreateSyncObjects() {
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start in signaled state
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkResult result1 = vkCreateSemaphore(m_device->GetDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
        VkResult result2 = vkCreateSemaphore(m_device->GetDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
        VkResult result3 = vkCreateFence(m_device->GetDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]);
        
        if (result1 != VK_SUCCESS || result2 != VK_SUCCESS || result3 != VK_SUCCESS) {
            std::cerr << "Failed to create synchronization objects for frame " << i << std::endl;
            return false;
        }
    }
    
    return true;
}

VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    // Prefer sRGB color space with BGRA format
    for (const auto& availableFormat : availableFormats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && 
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFormat;
        }
    }
    
    // Fallback to the first available format
    return availableFormats[0];
}

VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
    // If VSync is disabled, prefer mailbox mode for lower latency
    if (!m_initInfo.enableVSync) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }
        
        // Fallback to immediate mode if mailbox is not available
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                return availablePresentMode;
            }
        }
    }
    
    // VSync enabled or no other modes available - use FIFO (guaranteed to be available)
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        
        VkExtent2D actualExtent = {
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
        
        // Use preferred dimensions if specified
        if (m_initInfo.preferredWidth > 0 && m_initInfo.preferredHeight > 0) {
            actualExtent.width = m_initInfo.preferredWidth;
            actualExtent.height = m_initInfo.preferredHeight;
        }
        
        // Clamp to supported range
        actualExtent.width = std::clamp(actualExtent.width, 
                                       capabilities.minImageExtent.width, 
                                       capabilities.maxImageExtent.width);
        actualExtent.height = std::clamp(actualExtent.height, 
                                        capabilities.minImageExtent.height, 
                                        capabilities.maxImageExtent.height);
        
        return actualExtent;
    }
}

void VulkanSwapchain::CleanupSwapchain() {
    if (!m_device || m_device->GetDevice() == VK_NULL_HANDLE) {
        return;
    }
    
    // Cleanup image views
    for (auto imageView : m_swapchainImageViews) {
        vkDestroyImageView(m_device->GetDevice(), imageView, nullptr);
    }
    m_swapchainImageViews.clear();
    
    // Cleanup swapchain
    if (m_swapchain != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(m_device->GetDevice(), m_swapchain, nullptr);
        m_swapchain = VK_NULL_HANDLE;
    }
    
    // Clear image references (images are owned by swapchain)
    m_swapchainImages.clear();
}

void VulkanSwapchain::CleanupSyncObjects() {
    if (!m_device || m_device->GetDevice() == VK_NULL_HANDLE) {
        return;
    }
    
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (i < m_imageAvailableSemaphores.size()) {
            vkDestroySemaphore(m_device->GetDevice(), m_imageAvailableSemaphores[i], nullptr);
        }
        if (i < m_renderFinishedSemaphores.size()) {
            vkDestroySemaphore(m_device->GetDevice(), m_renderFinishedSemaphores[i], nullptr);
        }
        if (i < m_inFlightFences.size()) {
            vkDestroyFence(m_device->GetDevice(), m_inFlightFences[i], nullptr);
        }
    }
    
    m_imageAvailableSemaphores.clear();
    m_renderFinishedSemaphores.clear();
    m_inFlightFences.clear();
}