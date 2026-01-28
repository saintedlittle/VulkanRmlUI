#include "Engine.h"
#include "../Vulkan/VulkanRenderer.h"
#include "../Vulkan/ResourceManager.h"
#include "../UI/RmlUISystem.h"
#include "../Core/EventSystem.h"
#include "../Core/SceneManager.h"
#include "../Core/NavigationManager.h"
#include "../Assets/AssetManager.h"
#include "../Core/SettingsManager.h"
#include "../Audio/AudioManager.h"
#include "../Core/InputManager.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <thread>
#include <unordered_map>

Engine::Engine() = default;

Engine::~Engine() {
    if (m_initialized) {
        Shutdown();
    }
}

bool Engine::Initialize(const EngineConfig& config) {
    if (m_initialized) {
        std::cerr << "Engine already initialized!" << std::endl;
        return false;
    }
    
    try {
        // Initialize core modules in dependency order
        InitializeModules();
        
        // Set configuration through SettingsManager
        if (m_settingsManager) {
            m_settingsManager->SetConfig(config);
        }
        
        m_initialized = true;
        m_running = true;
        
        std::cout << "Engine initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Engine initialization failed: " << e.what() << std::endl;
        Shutdown();
        return false;
    }
}

void Engine::Run() {
    if (!m_initialized) {
        std::cerr << "Engine not initialized!" << std::endl;
        return;
    }
    
    auto lastTime = std::chrono::high_resolution_clock::now();
    
    while (m_running) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;
        
        // Update all modules
        UpdateModules(deltaTime);
        
        // Basic frame rate limiting (can be improved later)
        if (deltaTime < 0.016f) { // ~60 FPS
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}

void Engine::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    m_running = false;
    
    // Shutdown modules in reverse order
    ShutdownModules();
    
    m_initialized = false;
    std::cout << "Engine shutdown complete" << std::endl;
}

const EngineConfig& Engine::GetConfig() const {
    if (m_settingsManager) {
        return m_settingsManager->GetConfig();
    }
    
    // Return a static default config if SettingsManager is not available
    static EngineConfig defaultConfig;
    return defaultConfig;
}

void Engine::RegisterModule(std::unique_ptr<IEngineModule> module) {
    if (m_initialized) {
        std::cerr << "Cannot register modules after engine initialization!" << std::endl;
        return;
    }
    
    m_modules.push_back(std::move(module));
}

void Engine::InitializeModules() {
    // Initialize core modules in dependency order
    
    // 1. Event System (no dependencies)
    m_eventSystem = std::make_unique<EventSystem>();
    if (!m_eventSystem->Initialize()) {
        throw std::runtime_error("Failed to initialize EventSystem");
    }
    
    // 2. Settings Manager (depends on EventSystem)
    m_settingsManager = std::make_unique<SettingsManager>(m_eventSystem.get());
    if (!m_settingsManager->Initialize()) {
        throw std::runtime_error("Failed to initialize SettingsManager");
    }
    
    // 3. Vulkan Renderer (depends on Settings)
    m_renderer = std::make_unique<VulkanRenderer>(m_settingsManager.get());
    if (!m_renderer->Initialize()) {
        throw std::runtime_error("Failed to initialize VulkanRenderer");
    }
    
    // 3.5. Resource Manager (depends on Vulkan Renderer)
    m_resourceManager = std::make_unique<ResourceManager>(m_renderer.get());
    if (!m_resourceManager->Initialize()) {
        throw std::runtime_error("Failed to initialize ResourceManager");
    }
    
    // 4. Asset Manager (depends on Renderer and ResourceManager)
    m_assetManager = std::make_unique<AssetManager>(m_renderer.get(), m_resourceManager.get());
    // Set the correct asset base path for the executable location
    m_assetManager->SetAssetBasePath("assets/");
    if (!m_assetManager->Initialize()) {
        throw std::runtime_error("Failed to initialize AssetManager");
    }
    
    // 5. RmlUI System (depends on Renderer, ResourceManager, and AssetManager)
    m_uiSystem = std::make_unique<RmlUISystem>(m_renderer.get(), m_assetManager.get(), m_resourceManager.get());
    if (!m_uiSystem->Initialize()) {
        throw std::runtime_error("Failed to initialize RmlUISystem");
    }
    
    // 6. Scene Manager (depends on UI and Event systems)
    m_sceneManager = std::make_unique<SceneManager>(m_eventSystem.get(), m_uiSystem.get());
    if (!m_sceneManager->Initialize()) {
        throw std::runtime_error("Failed to initialize SceneManager");
    }
    
    // 7. Navigation Manager (depends on Scene Manager)
    m_navigationManager = std::make_unique<NavigationManager>(m_sceneManager.get(), m_eventSystem.get());
    if (!m_navigationManager->Initialize()) {
        throw std::runtime_error("Failed to initialize NavigationManager");
    }
    
    // 8. Audio Manager (depends on Settings)
    m_audioManager = std::make_unique<AudioManager>(m_settingsManager.get());
    if (!m_audioManager->Initialize()) {
        throw std::runtime_error("Failed to initialize AudioManager");
    }
    
    // 9. Input Manager (depends on Event System)
    m_inputManager = std::make_unique<InputManager>(m_eventSystem.get());
    if (!m_inputManager->Initialize()) {
        throw std::runtime_error("Failed to initialize InputManager");
    }
    
    // Initialize additional modules in order of their initialization priority
    std::sort(m_modules.begin(), m_modules.end(), 
        [](const std::unique_ptr<IEngineModule>& a, const std::unique_ptr<IEngineModule>& b) {
            return a->GetInitializationOrder() < b->GetInitializationOrder();
        });
    
    for (auto& module : m_modules) {
        if (!module->Initialize()) {
            throw std::runtime_error(std::string("Failed to initialize module: ") + module->GetName());
        }
    }
    
    // Set up settings change callbacks after all modules are initialized
    SetupSettingsCallbacks();
}

void Engine::SetupSettingsCallbacks() {
    if (!m_settingsManager) {
        return;
    }
    
    // Register VulkanRenderer for graphics settings changes
    if (m_renderer) {
        m_settingsManager->RegisterChangeCallback("graphics.windowWidth", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_renderer->OnSettingsChanged(key);
            });
        
        m_settingsManager->RegisterChangeCallback("graphics.windowHeight", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_renderer->OnSettingsChanged(key);
            });
        
        m_settingsManager->RegisterChangeCallback("graphics.fullscreen", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_renderer->OnSettingsChanged(key);
            });
        
        m_settingsManager->RegisterChangeCallback("graphics.vsync", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_renderer->OnSettingsChanged(key);
            });
        
        m_settingsManager->RegisterChangeCallback("graphics.msaaSamples", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_renderer->OnSettingsChanged(key);
            });
    }
    
    // Register AudioManager for audio settings changes
    if (m_audioManager) {
        m_settingsManager->RegisterChangeCallback("audio.masterVolume", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_audioManager->OnSettingsChanged(key);
            });
        
        m_settingsManager->RegisterChangeCallback("audio.musicVolume", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_audioManager->OnSettingsChanged(key);
            });
        
        m_settingsManager->RegisterChangeCallback("audio.sfxVolume", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_audioManager->OnSettingsChanged(key);
            });
        
        m_settingsManager->RegisterChangeCallback("audio.audioDevice", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_audioManager->OnSettingsChanged(key);
            });
    }
    
    // Register InputManager for input settings changes
    if (m_inputManager) {
        m_settingsManager->RegisterChangeCallback("input.mouseSensitivity", 
            [this](const std::string& key, const SettingsManager::SettingValue& value) {
                m_inputManager->OnSettingsChanged(key);
            });
    }
    
    std::cout << "Settings callbacks configured successfully" << std::endl;
}

void Engine::UpdateModules(float deltaTime) {
    // Update core modules
    if (m_eventSystem) m_eventSystem->Update(deltaTime);
    if (m_settingsManager) m_settingsManager->Update(deltaTime);
    if (m_renderer) m_renderer->Update(deltaTime);
    if (m_resourceManager) m_resourceManager->Update(deltaTime);
    if (m_assetManager) m_assetManager->Update(deltaTime);
    if (m_uiSystem) m_uiSystem->Update(deltaTime);
    if (m_sceneManager) m_sceneManager->Update(deltaTime);
    if (m_navigationManager) m_navigationManager->Update(deltaTime);
    if (m_audioManager) m_audioManager->Update(deltaTime);
    if (m_inputManager) m_inputManager->Update(deltaTime);
    
    // Update additional modules
    for (auto& module : m_modules) {
        module->Update(deltaTime);
    }
}

void Engine::ShutdownModules() {
    // Shutdown additional modules first (reverse order)
    for (auto it = m_modules.rbegin(); it != m_modules.rend(); ++it) {
        (*it)->Shutdown();
    }
    m_modules.clear();
    
    // Shutdown core modules in reverse dependency order
    if (m_inputManager) {
        m_inputManager->Shutdown();
        m_inputManager.reset();
    }
    
    if (m_audioManager) {
        m_audioManager->Shutdown();
        m_audioManager.reset();
    }
    
    if (m_navigationManager) {
        m_navigationManager->Shutdown();
        m_navigationManager.reset();
    }
    
    if (m_sceneManager) {
        m_sceneManager->Shutdown();
        m_sceneManager.reset();
    }
    
    if (m_uiSystem) {
        m_uiSystem->Shutdown();
        m_uiSystem.reset();
    }
    
    if (m_assetManager) {
        m_assetManager->Shutdown();
        m_assetManager.reset();
    }
    
    if (m_resourceManager) {
        m_resourceManager->Shutdown();
        m_resourceManager.reset();
    }
    
    if (m_renderer) {
        m_renderer->Shutdown();
        m_renderer.reset();
    }
    
    if (m_settingsManager) {
        m_settingsManager->Shutdown();
        m_settingsManager.reset();
    }
    
    if (m_eventSystem) {
        m_eventSystem->Shutdown();
        m_eventSystem.reset();
    }
}