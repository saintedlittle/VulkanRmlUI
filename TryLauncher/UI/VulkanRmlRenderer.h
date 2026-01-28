#pragma once

#include <RmlUi/Core/RenderInterface.h>
#include <vulkan/vulkan.h>
#include <vk_mem_alloc.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <unordered_map>
#include <array>
#include "../Vulkan/ResourceManager.h"

// Forward declarations
class VulkanRenderer;

/**
 * VulkanRmlRenderer implements RmlUI's RenderInterface for Vulkan backend.
 * This class handles:
 * - UI geometry rendering with vertex/index buffers
 * - Texture loading and management for UI elements
 * - Transform and scissor region management
 * - UI-specific Vulkan pipeline and descriptor sets
 */
class VulkanRmlRenderer : public Rml::RenderInterface {
public:
    VulkanRmlRenderer(VulkanRenderer* renderer, ResourceManager* resourceManager);
    ~VulkanRmlRenderer();

    // Initialization and cleanup
    bool Initialize();
    void Cleanup();

    // Frame management
    void BeginFrame(VkCommandBuffer commandBuffer, VkRenderPass renderPass, 
                   uint32_t framebufferWidth, uint32_t framebufferHeight);
    void EndFrame();

    // RenderInterface implementation
    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
    void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;

    // Optional methods
    void SetTransform(const Rml::Matrix4f* transform) override;

private:
    // Compiled geometry structure
    struct CompiledGeometry {
        AllocatedBuffer vertexBuffer;
        AllocatedBuffer indexBuffer;
        uint32_t indexCount;
        uint32_t vertexCount;
    };

    // Vertex structure for UI rendering
    struct UIVertex {
        glm::vec2 position;
        glm::vec4 color;
        glm::vec2 texCoord;
        
        static VkVertexInputBindingDescription GetBindingDescription();
        static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions();
    };

    // Push constants for UI rendering
    struct UIPushConstants {
        glm::mat4 transform;
        glm::vec2 translation;
        int useTexture;
        float _padding;
    };

    // Texture resource wrapper
    struct TextureResource {
        AllocatedImage image;
        VkSampler sampler = VK_NULL_HANDLE;
        uint32_t width = 0;
        uint32_t height = 0;
    };

    // Initialization helpers
    bool CreatePipeline();
    bool CreateDescriptorSetLayout();
    bool CreateDescriptorPool();
    bool CreateSampler();
    bool CreateDefaultTexture();

    // Resource management
    void UpdateVertexBuffer(Rml::Vertex* vertices, int num_vertices);
    void UpdateIndexBuffer(int* indices, int num_indices);
    TextureResource* CreateTextureFromData(const Rml::byte* data, int width, int height, int channels);
    TextureResource* LoadTextureFromFile(const std::string& path);

    // Rendering helpers
    void BindPipeline();
    void UpdateDescriptorSet(Rml::TextureHandle texture);
    void DrawGeometry(int num_indices);

    VulkanRenderer* m_renderer;
    ResourceManager* m_resourceManager;

    // Vulkan objects
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet m_descriptorSet = VK_NULL_HANDLE;
    VkSampler m_defaultSampler = VK_NULL_HANDLE;

    // Buffers
    AllocatedBuffer m_vertexBuffer;
    AllocatedBuffer m_indexBuffer;
    uint32_t m_maxVertices = 10000;
    uint32_t m_maxIndices = 30000;

    // Textures
    std::unordered_map<Rml::TextureHandle, std::unique_ptr<TextureResource>> m_textures;
    Rml::TextureHandle m_nextTextureHandle = 1;
    TextureResource* m_defaultTexture = nullptr;

    // Compiled geometry
    std::unordered_map<Rml::CompiledGeometryHandle, std::unique_ptr<CompiledGeometry>> m_geometries;
    Rml::CompiledGeometryHandle m_nextGeometryHandle = 1;

    // Render state
    VkCommandBuffer m_currentCommandBuffer = VK_NULL_HANDLE;
    VkRenderPass m_currentRenderPass = VK_NULL_HANDLE;
    uint32_t m_framebufferWidth = 0;
    uint32_t m_framebufferHeight = 0;

    // Transform state
    glm::mat4 m_currentTransform = glm::mat4(1.0f);
    bool m_scissorEnabled = false;
    VkRect2D m_scissorRect = {};

    bool m_initialized = false;
};