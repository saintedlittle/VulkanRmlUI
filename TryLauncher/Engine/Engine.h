#pragma once

#include "../Core/EngineConfig.h"
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
class AudioManager;
class InputManager;

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
    AudioManager* GetAudioManager() const { return m_audioManager.get(); }
    InputManager* GetInputManager() const { return m_inputManager.get(); }
    
    // Module registration
    void RegisterModule(std::unique_ptr<IEngineModule> module);
    
    // Engine state
    bool IsRunning() const { return m_running; }
    void RequestShutdown() { m_running = false; }
    
    // Configuration
    const EngineConfig& GetConfig() const;

private:
    void InitializeModules();
    void SetupSettingsCallbacks();
    void UpdateModules(float deltaTime);
    void ShutdownModules();
    
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
    std::unique_ptr<AudioManager> m_audioManager;
    std::unique_ptr<InputManager> m_inputManager;
    
    // Additional modules
    std::vector<std::unique_ptr<IEngineModule>> m_modules;
};