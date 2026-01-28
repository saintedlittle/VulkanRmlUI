#include "VulkanRmlRenderer.h"
#include "../Vulkan/VulkanRenderer.h"
#include "../Vulkan/ResourceManager.h"
#include "../Vulkan/VulkanDevice.h"
#include <iostream>
#include <array>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

VulkanRmlRenderer::VulkanRmlRenderer(VulkanRenderer* renderer, ResourceManager* resourceManager)
    : m_renderer(renderer), m_resourceManager(resourceManager) {
}

VulkanRmlRenderer::~VulkanRmlRenderer() {
    if (m_initialized) {
        Cleanup();
    }
}

bool VulkanRmlRenderer::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing VulkanRmlRenderer..." << std::endl;

    try {
        // Create descriptor set layout
        if (!CreateDescriptorSetLayout()) {
            std::cerr << "Failed to create descriptor set layout" << std::endl;
            return false;
        }

        // Create pipeline
        if (!CreatePipeline()) {
            std::cerr << "Failed to create pipeline" << std::endl;
            return false;
        }

        // Create descriptor pool
        if (!CreateDescriptorPool()) {
            std::cerr << "Failed to create descriptor pool" << std::endl;
            return false;
        }

        // Create sampler
        if (!CreateSampler()) {
            std::cerr << "Failed to create sampler" << std::endl;
            return false;
        }

        // Create vertex buffer
        m_vertexBuffer = m_resourceManager->CreateBuffer(
            sizeof(UIVertex) * m_maxVertices,
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );

        if (!m_vertexBuffer.IsValid()) {
            std::cerr << "Failed to create vertex buffer" << std::endl;
            return false;
        }

        // Create index buffer
        m_indexBuffer = m_resourceManager->CreateBuffer(
            sizeof(uint32_t) * m_maxIndices,
            VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_MAPPED_BIT
        );

        if (!m_indexBuffer.IsValid()) {
            std::cerr << "Failed to create index buffer" << std::endl;
            return false;
        }

        // Create default texture
        if (!CreateDefaultTexture()) {
            std::cerr << "Failed to create default texture" << std::endl;
            return false;
        }

        m_initialized = true;
        std::cout << "VulkanRmlRenderer initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "VulkanRmlRenderer initialization failed: " << e.what() << std::endl;
        Cleanup();
        return false;
    }
}

void VulkanRmlRenderer::Cleanup() {
    if (!m_initialized) {
        return;
    }

    VkDevice device = m_renderer->GetDevice();

    // Wait for device to be idle
    vkDeviceWaitIdle(device);

    // Cleanup geometries
    for (auto& [handle, geometry] : m_geometries) {
        m_resourceManager->DestroyBuffer(geometry->vertexBuffer);
        m_resourceManager->DestroyBuffer(geometry->indexBuffer);
    }
    m_geometries.clear();

    // Cleanup textures
    for (auto& [handle, texture] : m_textures) {
        if (texture->sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, texture->sampler, nullptr);
        }
        m_resourceManager->DestroyImage(texture->image);
    }
    m_textures.clear();

    // Cleanup buffers
    if (m_vertexBuffer.IsValid()) {
        m_resourceManager->DestroyBuffer(m_vertexBuffer);
    }
    if (m_indexBuffer.IsValid()) {
        m_resourceManager->DestroyBuffer(m_indexBuffer);
    }

    // Cleanup Vulkan objects
    if (m_defaultSampler != VK_NULL_HANDLE) {
        vkDestroySampler(device, m_defaultSampler, nullptr);
        m_defaultSampler = VK_NULL_HANDLE;
    }

    if (m_descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, m_descriptorPool, nullptr);
        m_descriptorPool = VK_NULL_HANDLE;
    }

    if (m_pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }

    if (m_pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }

    if (m_descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, m_descriptorSetLayout, nullptr);
        m_descriptorSetLayout = VK_NULL_HANDLE;
    }

    m_initialized = false;
    std::cout << "VulkanRmlRenderer cleanup complete" << std::endl;
}

void VulkanRmlRenderer::BeginFrame(VkCommandBuffer commandBuffer, VkRenderPass renderPass,
                                  uint32_t framebufferWidth, uint32_t framebufferHeight) {
    m_currentCommandBuffer = commandBuffer;
    m_currentRenderPass = renderPass;
    m_framebufferWidth = framebufferWidth;
    m_framebufferHeight = framebufferHeight;

    // Set up viewport and scissor
    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(framebufferWidth);
    viewport.height = static_cast<float>(framebufferHeight);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = {framebufferWidth, framebufferHeight};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // Set up projection matrix for UI rendering
    m_currentTransform = glm::ortho(0.0f, static_cast<float>(framebufferWidth),
                                   static_cast<float>(framebufferHeight), 0.0f,
                                   -1.0f, 1.0f);
}

void VulkanRmlRenderer::EndFrame() {
    m_currentCommandBuffer = VK_NULL_HANDLE;
    m_currentRenderPass = VK_NULL_HANDLE;
}

Rml::CompiledGeometryHandle VulkanRmlRenderer::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    if (!m_initialized) {
        return 0;
    }

    // Create compiled geometry
    auto geometry = std::make_unique<CompiledGeometry>();
    
    // Create vertex buffer
    VkDeviceSize vertexBufferSize = sizeof(UIVertex) * vertices.size();
    geometry->vertexBuffer = m_resourceManager->CreateBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_MAPPED_BIT
    );
    
    if (!geometry->vertexBuffer.IsValid()) {
        return 0;
    }
    
    // Create index buffer
    VkDeviceSize indexBufferSize = sizeof(uint32_t) * indices.size();
    geometry->indexBuffer = m_resourceManager->CreateBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_MAPPED_BIT
    );
    
    if (!geometry->indexBuffer.IsValid()) {
        m_resourceManager->DestroyBuffer(geometry->vertexBuffer);
        return 0;
    }
    
    // Convert and copy vertex data
    std::vector<UIVertex> uiVertices(vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
        const Rml::Vertex& rmlVertex = vertices[i];
        UIVertex& uiVertex = uiVertices[i];
        
        uiVertex.position = glm::vec2(rmlVertex.position.x, rmlVertex.position.y);
        uiVertex.color = glm::vec4(
            rmlVertex.colour.red / 255.0f,
            rmlVertex.colour.green / 255.0f,
            rmlVertex.colour.blue / 255.0f,
            rmlVertex.colour.alpha / 255.0f
        );
        uiVertex.texCoord = glm::vec2(rmlVertex.tex_coord.x, rmlVertex.tex_coord.y);
    }
    
    // Copy vertex data to buffer
    void* vertexData = m_resourceManager->MapBuffer(geometry->vertexBuffer);
    if (vertexData) {
        memcpy(vertexData, uiVertices.data(), vertexBufferSize);
        m_resourceManager->FlushBuffer(geometry->vertexBuffer, 0, vertexBufferSize);
        m_resourceManager->UnmapBuffer(geometry->vertexBuffer);
    }
    
    // Copy index data to buffer
    void* indexData = m_resourceManager->MapBuffer(geometry->indexBuffer);
    if (indexData) {
        memcpy(indexData, indices.data(), indexBufferSize);
        m_resourceManager->FlushBuffer(geometry->indexBuffer, 0, indexBufferSize);
        m_resourceManager->UnmapBuffer(geometry->indexBuffer);
    }
    
    geometry->vertexCount = static_cast<uint32_t>(vertices.size());
    geometry->indexCount = static_cast<uint32_t>(indices.size());
    
    // Store geometry and return handle
    Rml::CompiledGeometryHandle handle = m_nextGeometryHandle++;
    m_geometries[handle] = std::move(geometry);
    
    return handle;
}

void VulkanRmlRenderer::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) {
    if (!m_initialized || !m_currentCommandBuffer) {
        return;
    }
    
    auto it = m_geometries.find(geometry);
    if (it == m_geometries.end()) {
        return;
    }
    
    const CompiledGeometry* geom = it->second.get();
    
    // Bind pipeline (placeholder - needs actual pipeline implementation)
    // BindPipeline();
    
    // Bind vertex buffer
    VkBuffer vertexBuffers[] = {geom->vertexBuffer.buffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, vertexBuffers, offsets);
    
    // Bind index buffer
    vkCmdBindIndexBuffer(m_currentCommandBuffer, geom->indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    
    // Set push constants
    UIPushConstants pushConstants = {};
    pushConstants.transform = m_currentTransform;
    pushConstants.translation = glm::vec2(translation.x, translation.y);
    pushConstants.useTexture = (texture != 0) ? 1 : 0;
    
    // TODO: Implement actual pipeline and push constants
    // vkCmdPushConstants(m_currentCommandBuffer, m_pipelineLayout,
    //                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
    //                   0, sizeof(UIPushConstants), &pushConstants);
    
    // Apply scissor if enabled
    if (m_scissorEnabled) {
        vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &m_scissorRect);
    }
    
    // Draw indexed
    vkCmdDrawIndexed(m_currentCommandBuffer, geom->indexCount, 1, 0, 0, 0);
}

void VulkanRmlRenderer::ReleaseGeometry(Rml::CompiledGeometryHandle geometry) {
    auto it = m_geometries.find(geometry);
    if (it != m_geometries.end()) {
        // Cleanup buffers
        m_resourceManager->DestroyBuffer(it->second->vertexBuffer);
        m_resourceManager->DestroyBuffer(it->second->indexBuffer);
        
        // Remove from map
        m_geometries.erase(it);
    }
}

Rml::TextureHandle VulkanRmlRenderer::LoadTexture(Rml::Vector2i& texture_dimensions,
                                                 const Rml::String& source) {
    if (!m_initialized) {
        return 0;
    }

    // Try to load texture from file
    TextureResource* texture = LoadTextureFromFile(source);
    if (!texture) {
        std::cerr << "Failed to load texture: " << source << std::endl;
        return 0;
    }

    texture_dimensions.x = static_cast<int>(texture->width);
    texture_dimensions.y = static_cast<int>(texture->height);

    Rml::TextureHandle handle = m_nextTextureHandle++;
    m_textures[handle] = std::unique_ptr<TextureResource>(texture);

    return handle;
}

Rml::TextureHandle VulkanRmlRenderer::GenerateTexture(Rml::Span<const Rml::byte> source,
                                                     Rml::Vector2i source_dimensions) {
    if (!m_initialized) {
        return 0;
    }

    // Create texture from raw data
    TextureResource* texture = CreateTextureFromData(source.data(),
                                                    source_dimensions.x,
                                                    source_dimensions.y,
                                                    4); // Assume RGBA

    if (!texture) {
        std::cerr << "Failed to generate texture from data" << std::endl;
        return 0;
    }

    Rml::TextureHandle handle = m_nextTextureHandle++;
    m_textures[handle] = std::unique_ptr<TextureResource>(texture);

    return handle;
}

void VulkanRmlRenderer::ReleaseTexture(Rml::TextureHandle texture) {
    auto it = m_textures.find(texture);
    if (it != m_textures.end()) {
        VkDevice device = m_renderer->GetDevice();
        if (it->second->sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, it->second->sampler, nullptr);
        }
        m_resourceManager->DestroyImage(it->second->image);
        m_textures.erase(it);
    }
}

void VulkanRmlRenderer::SetTransform(const Rml::Matrix4f* transform) {
    if (transform) {
        // Convert RmlUI matrix to glm matrix using data() method
        const float* matrixData = transform->data();
        m_currentTransform = glm::mat4(
            matrixData[0], matrixData[4], matrixData[8],  matrixData[12],
            matrixData[1], matrixData[5], matrixData[9],  matrixData[13],
            matrixData[2], matrixData[6], matrixData[10], matrixData[14],
            matrixData[3], matrixData[7], matrixData[11], matrixData[15]
        );
    } else {
        // Reset to orthographic projection
        m_currentTransform = glm::ortho(0.0f, static_cast<float>(m_framebufferWidth),
                                       static_cast<float>(m_framebufferHeight), 0.0f,
                                       -1.0f, 1.0f);
    }
}

void VulkanRmlRenderer::EnableScissorRegion(bool enable) {
    m_scissorEnabled = enable;
}

void VulkanRmlRenderer::SetScissorRegion(Rml::Rectanglei region) {
    m_scissorRect.offset.x = std::max(0, region.Left());
    m_scissorRect.offset.y = std::max(0, region.Top());
    m_scissorRect.extent.width = std::max(0, static_cast<int>(region.Width()));
    m_scissorRect.extent.height = std::max(0, static_cast<int>(region.Height()));

    // Clamp to framebuffer bounds
    m_scissorRect.extent.width = std::min(m_scissorRect.extent.width,
                                         m_framebufferWidth - m_scissorRect.offset.x);
    m_scissorRect.extent.height = std::min(m_scissorRect.extent.height,
                                          m_framebufferHeight - m_scissorRect.offset.y);

    if (m_currentCommandBuffer && m_scissorEnabled) {
        vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &m_scissorRect);
    }
}

// Private helper methods implementation will continue in next part...
// Private helper methods implementation

VkVertexInputBindingDescription VulkanRmlRenderer::UIVertex::GetBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(UIVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

std::array<VkVertexInputAttributeDescription, 3> VulkanRmlRenderer::UIVertex::GetAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

    // Position
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(UIVertex, position);

    // Color
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(UIVertex, color);

    // Texture coordinates
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(UIVertex, texCoord);

    return attributeDescriptions;
}

bool VulkanRmlRenderer::CreateDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    VkResult result = vkCreateDescriptorSetLayout(m_renderer->GetDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout);
    return result == VK_SUCCESS;
}

bool VulkanRmlRenderer::CreatePipeline() {
    // Create shader modules (placeholder - you'll need actual shader code)
    const char* vertexShaderCode = R"(
        #version 450
        
        layout(location = 0) in vec2 inPosition;
        layout(location = 1) in vec4 inColor;
        layout(location = 2) in vec2 inTexCoord;
        
        layout(push_constant) uniform PushConstants {
            mat4 transform;
            vec2 translation;
            int useTexture;
        } pc;
        
        layout(location = 0) out vec4 fragColor;
        layout(location = 1) out vec2 fragTexCoord;
        layout(location = 2) out flat int fragUseTexture;
        
        void main() {
            vec2 pos = inPosition + pc.translation;
            gl_Position = pc.transform * vec4(pos, 0.0, 1.0);
            fragColor = inColor;
            fragTexCoord = inTexCoord;
            fragUseTexture = pc.useTexture;
        }
    )";

    const char* fragmentShaderCode = R"(
        #version 450
        
        layout(location = 0) in vec4 fragColor;
        layout(location = 1) in vec2 fragTexCoord;
        layout(location = 2) in flat int fragUseTexture;
        
        layout(binding = 0) uniform sampler2D texSampler;
        
        layout(location = 0) out vec4 outColor;
        
        void main() {
            if (fragUseTexture != 0) {
                outColor = fragColor * texture(texSampler, fragTexCoord);
            } else {
                outColor = fragColor;
            }
        }
    )";

    // TODO: Compile shaders and create pipeline
    // For now, return true as placeholder
    std::cout << "Pipeline creation placeholder - implement shader compilation" << std::endl;
    return true;
}

bool VulkanRmlRenderer::CreateDescriptorPool() {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = 1000; // Support many textures

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1000;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    VkResult result = vkCreateDescriptorPool(m_renderer->GetDevice(), &poolInfo, nullptr, &m_descriptorPool);
    return result == VK_SUCCESS;
}

bool VulkanRmlRenderer::CreateSampler() {
    VkSamplerCreateInfo samplerInfo = {};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkResult result = vkCreateSampler(m_renderer->GetDevice(), &samplerInfo, nullptr, &m_defaultSampler);
    return result == VK_SUCCESS;
}

bool VulkanRmlRenderer::CreateDefaultTexture() {
    // Create a 1x1 white texture as default
    const uint32_t whitePixel = 0xFFFFFFFF;
    
    m_defaultTexture = CreateTextureFromData(reinterpret_cast<const Rml::byte*>(&whitePixel), 1, 1, 4);
    return m_defaultTexture != nullptr;
}

void VulkanRmlRenderer::UpdateVertexBuffer(Rml::Vertex* vertices, int num_vertices) {
    if (!m_vertexBuffer.IsValid() || num_vertices == 0) {
        return;
    }

    // Convert RmlUI vertices to our format
    std::vector<UIVertex> uiVertices(num_vertices);
    for (int i = 0; i < num_vertices; ++i) {
        const Rml::Vertex& rmlVertex = vertices[i];
        UIVertex& uiVertex = uiVertices[i];
        
        uiVertex.position = glm::vec2(rmlVertex.position.x, rmlVertex.position.y);
        uiVertex.color = glm::vec4(
            rmlVertex.colour.red / 255.0f,
            rmlVertex.colour.green / 255.0f,
            rmlVertex.colour.blue / 255.0f,
            rmlVertex.colour.alpha / 255.0f
        );
        uiVertex.texCoord = glm::vec2(rmlVertex.tex_coord.x, rmlVertex.tex_coord.y);
    }

    // Copy to buffer
    void* data = m_resourceManager->MapBuffer(m_vertexBuffer);
    if (data) {
        memcpy(data, uiVertices.data(), sizeof(UIVertex) * num_vertices);
        m_resourceManager->FlushBuffer(m_vertexBuffer, 0, sizeof(UIVertex) * num_vertices);
        m_resourceManager->UnmapBuffer(m_vertexBuffer);
    }
}

void VulkanRmlRenderer::UpdateIndexBuffer(int* indices, int num_indices) {
    if (!m_indexBuffer.IsValid() || num_indices == 0) {
        return;
    }

    // Copy indices to buffer
    void* data = m_resourceManager->MapBuffer(m_indexBuffer);
    if (data) {
        memcpy(data, indices, sizeof(uint32_t) * num_indices);
        m_resourceManager->FlushBuffer(m_indexBuffer, 0, sizeof(uint32_t) * num_indices);
        m_resourceManager->UnmapBuffer(m_indexBuffer);
    }
}

VulkanRmlRenderer::TextureResource* VulkanRmlRenderer::CreateTextureFromData(const Rml::byte* data, int width, int height, int channels) {
    if (!data || width <= 0 || height <= 0) {
        return nullptr;
    }

    // Create staging buffer
    VkDeviceSize imageSize = width * height * channels;
    AllocatedBuffer stagingBuffer = m_resourceManager->CreateStagingBuffer(imageSize);
    if (!stagingBuffer.IsValid()) {
        return nullptr;
    }

    // Copy data to staging buffer
    void* stagingData = m_resourceManager->MapBuffer(stagingBuffer);
    if (stagingData) {
        memcpy(stagingData, data, imageSize);
        m_resourceManager->UnmapBuffer(stagingBuffer);
    }

    // Create image
    VkFormat format = (channels == 4) ? VK_FORMAT_R8G8B8A8_UNORM : VK_FORMAT_R8G8B8_UNORM;
    AllocatedImage image = m_resourceManager->CreateTexture2D(width, height, format);
    if (!image.IsValid()) {
        m_resourceManager->DestroyBuffer(stagingBuffer);
        return nullptr;
    }

    // Transition image layout and copy data
    m_resourceManager->TransitionImageLayout(image.image, format,
                                           VK_IMAGE_LAYOUT_UNDEFINED,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    m_resourceManager->CopyBufferToImage(stagingBuffer, image, width, height);

    m_resourceManager->TransitionImageLayout(image.image, format,
                                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                           VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Cleanup staging buffer
    m_resourceManager->DestroyBuffer(stagingBuffer);

    // Create texture resource
    auto texture = std::make_unique<TextureResource>();
    texture->image = image;
    texture->sampler = m_defaultSampler; // Use shared sampler
    texture->width = width;
    texture->height = height;

    return texture.release();
}

VulkanRmlRenderer::TextureResource* VulkanRmlRenderer::LoadTextureFromFile(const std::string& path) {
    int width, height, channels;
    stbi_uc* pixels = stbi_load(path.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    
    if (!pixels) {
        std::cerr << "Failed to load texture: " << path << std::endl;
        return nullptr;
    }

    TextureResource* texture = CreateTextureFromData(pixels, width, height, 4);
    stbi_image_free(pixels);

    return texture;
}

void VulkanRmlRenderer::BindPipeline() {
    if (m_pipeline != VK_NULL_HANDLE && m_currentCommandBuffer) {
        vkCmdBindPipeline(m_currentCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
        
        // Bind vertex buffer
        VkBuffer vertexBuffers[] = {m_vertexBuffer.buffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(m_currentCommandBuffer, 0, 1, vertexBuffers, offsets);
        
        // Bind index buffer
        vkCmdBindIndexBuffer(m_currentCommandBuffer, m_indexBuffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    }
}

void VulkanRmlRenderer::UpdateDescriptorSet(Rml::TextureHandle texture) {
    // TODO: Implement descriptor set updates for textures
    // This is a placeholder implementation
}

void VulkanRmlRenderer::DrawGeometry(int num_indices) {
    if (m_currentCommandBuffer && num_indices > 0) {
        // Apply scissor if enabled
        if (m_scissorEnabled) {
            vkCmdSetScissor(m_currentCommandBuffer, 0, 1, &m_scissorRect);
        }
        
        // Draw indexed
        vkCmdDrawIndexed(m_currentCommandBuffer, num_indices, 1, 0, 0, 0);
    }
}