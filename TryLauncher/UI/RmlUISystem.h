#pragma once

#include "../Engine/Engine.h"
#include <RmlUi/Core.h>
#include <vulkan/vulkan.h>
#include <memory>
#include <string>
#include <unordered_map>

class VulkanRenderer;
class AssetManager;
class VulkanRmlRenderer;
class ResourceManager;

/**
 * RmlUISystem manages the RmlUI integration with the Vulkan renderer.
 * This class handles:
 * - RmlUI initialization with custom Vulkan backend
 * - Context creation and document management
 * - Font loading and management integration
 * - Input event processing and routing to RmlUI
 */
class RmlUISystem : public IEngineModule {
public:
    RmlUISystem(VulkanRenderer* renderer, AssetManager* assetManager, ResourceManager* resourceManager);
    ~RmlUISystem();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "RmlUISystem"; }
    int GetInitializationOrder() const override { return 500; }

    // RmlUI-specific methods
    bool LoadFont(const std::string& fontPath, const std::string& fontName);
    bool LoadStylesheet(const std::string& stylesheetPath);
    
    // Context management
    Rml::Context* GetContext() const { return m_context; }
    
    // Document management
    Rml::ElementDocument* LoadDocument(const std::string& rmlPath);
    void UnloadDocument(Rml::ElementDocument* document);
    void ShowDocument(Rml::ElementDocument* document);
    void HideDocument(Rml::ElementDocument* document);
    
    // UIDocument factory
    std::unique_ptr<class UIDocument> CreateUIDocument();
    
    // Input event processing
    void ProcessKeyEvent(int key, int action, int mods);
    void ProcessMouseButtonEvent(int button, int action, int mods);
    void ProcessMouseMoveEvent(double xpos, double ypos);
    void ProcessScrollEvent(double xoffset, double yoffset);
    void ProcessCharEvent(unsigned int codepoint);
    
    // Rendering
    void Render(VkCommandBuffer commandBuffer, VkRenderPass renderPass, 
               uint32_t framebufferWidth, uint32_t framebufferHeight);

private:
    // RmlUI system interface implementations
    class SystemInterface;
    class FileInterface;
    
    // Initialization helpers
    bool InitializeRmlUI();
    bool CreateContext();
    void SetupEventHandlers();
    
    // Input conversion helpers
    Rml::Input::KeyIdentifier ConvertKey(int glfwKey);
    int ConvertKeyModifiers(int glfwMods);
    
    VulkanRenderer* m_renderer;
    AssetManager* m_assetManager;
    ResourceManager* m_resourceManager;
    
    // RmlUI objects
    std::unique_ptr<VulkanRmlRenderer> m_rmlRenderer;
    std::unique_ptr<SystemInterface> m_systemInterface;
    std::unique_ptr<FileInterface> m_fileInterface;
    Rml::Context* m_context = nullptr;
    
    // Document management
    std::unordered_map<std::string, Rml::ElementDocument*> m_loadedDocuments;
    
    // Input state
    double m_mouseX = 0.0;
    double m_mouseY = 0.0;
    
    bool m_initialized = false;
};