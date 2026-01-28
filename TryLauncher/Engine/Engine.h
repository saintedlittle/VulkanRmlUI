#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <string>

// Forward declarations
class VulkanRenderer;
class RmlUISystem;
class EventSystem;
class SceneManager;
class NavigationManager;
class AssetManager;
class SettingsManager;
class ResourceManager;

struct EngineConfig {
    struct Graphics {
        uint32_t windowWidth = 1920;
        uint32_t windowHeight = 1080;
        bool fullscreen = false;
        bool vsync = true;
        uint32_t msaaSamples = 1;
        bool enableValidation = false;
    } graphics;
    
    struct Audio {
        float masterVolume = 1.0f;
        float musicVolume = 0.8f;
        float sfxVolume = 1.0f;
        std::string audioDevice = "default";
    } audio;
    
    struct Input {
        std::unordered_map<std::string, int> keyBindings;
        float mouseSensitivity = 1.0f;
    } input;
    
    std::string assetPath = "assets/";
    std::string configPath = "config.json";
};

class IEngineModule {
public:
    virtual ~IEngineModule() = default;
    virtual bool Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Shutdown() = 0;
    virtual const char* GetName() const = 0;
    virtual int GetInitializationOrder() const = 0;
};

class Engine {
public:
    Engine();
    ~Engine();

    bool Initialize(const EngineConfig& config);
    void Run();
    void Shutdown();
    
    // Module access
    VulkanRenderer* GetRenderer() const { return m_renderer.get(); }
    RmlUISystem* GetUISystem() const { return m_uiSystem.get(); }
    EventSystem* GetEventSystem() const { return m_eventSystem.get(); }
    SceneManager* GetSceneManager() const { return m_sceneManager.get(); }
    NavigationManager* GetNavigationManager() const { return m_navigationManager.get(); }
    AssetManager* GetAssetManager() const { return m_assetManager.get(); }
    SettingsManager* GetSettingsManager() const { return m_settingsManager.get(); }
    ResourceManager* GetResourceManager() const { return m_resourceManager.get(); }
    
    // Module registration
    void RegisterModule(std::unique_ptr<IEngineModule> module);
    
    // Engine state
    bool IsRunning() const { return m_running; }
    void RequestShutdown() { m_running = false; }
    
    // Configuration
    const EngineConfig& GetConfig() const { return m_config; }

private:
    void InitializeModules();
    void UpdateModules(float deltaTime);
    void ShutdownModules();
    
    EngineConfig m_config;
    bool m_running = false;
    bool m_initialized = false;
    
    // Core modules
    std::unique_ptr<VulkanRenderer> m_renderer;
    std::unique_ptr<ResourceManager> m_resourceManager;
    std::unique_ptr<RmlUISystem> m_uiSystem;
    std::unique_ptr<EventSystem> m_eventSystem;
    std::unique_ptr<SceneManager> m_sceneManager;
    std::unique_ptr<NavigationManager> m_navigationManager;
    std::unique_ptr<AssetManager> m_assetManager;
    std::unique_ptr<SettingsManager> m_settingsManager;
    
    // Additional modules
    std::vector<std::unique_ptr<IEngineModule>> m_modules;
};