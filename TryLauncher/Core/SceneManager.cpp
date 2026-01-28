#include "SceneManager.h"
#include "EventSystem.h"
#include "../UI/RmlUISystem.h"
#include <iostream>

SceneManager::SceneManager(EventSystem* eventSystem, RmlUISystem* uiSystem)
    : m_eventSystem(eventSystem), m_uiSystem(uiSystem) {
}

SceneManager::~SceneManager() {
    if (m_initialized) {
        Shutdown();
    }
}

bool SceneManager::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing SceneManager..." << std::endl;
    
    // TODO: Set up scene management in later tasks
    
    m_initialized = true;
    std::cout << "SceneManager initialized (placeholder)" << std::endl;
    return true;
}

void SceneManager::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }
    
    UpdateCurrentScene(deltaTime);
}

void SceneManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down SceneManager..." << std::endl;
    
    // Exit current scene
    if (m_currentScene) {
        m_currentScene->OnExit();
        m_currentScene->Cleanup();
        m_currentScene = nullptr;
    }
    
    // Clean up all scenes
    m_scenes.clear();
    m_currentSceneName.clear();
    
    m_initialized = false;
    std::cout << "SceneManager shutdown complete" << std::endl;
}

void SceneManager::RegisterScene(const std::string& name, std::unique_ptr<Scene> scene) {
    if (m_scenes.find(name) != m_scenes.end()) {
        std::cerr << "Scene '" << name << "' already registered!" << std::endl;
        return;
    }
    
    m_scenes[name] = std::move(scene);
    std::cout << "Registered scene: " << name << std::endl;
}

bool SceneManager::SwitchToScene(const std::string& name) {
    auto it = m_scenes.find(name);
    if (it == m_scenes.end()) {
        std::cerr << "Scene '" << name << "' not found!" << std::endl;
        return false;
    }
    
    // Exit current scene
    if (m_currentScene) {
        m_currentScene->OnExit();
        m_currentScene->Cleanup();
    }
    
    // Switch to new scene
    m_currentScene = it->second.get();
    m_currentSceneName = name;
    
    // Initialize and enter new scene
    if (!m_currentScene->Initialize()) {
        std::cerr << "Failed to initialize scene: " << name << std::endl;
        m_currentScene = nullptr;
        m_currentSceneName.clear();
        return false;
    }
    
    m_currentScene->OnEnter();
    std::cout << "Switched to scene: " << name << std::endl;
    return true;
}

void SceneManager::UpdateCurrentScene(float deltaTime) {
    if (m_currentScene) {
        m_currentScene->Update(deltaTime);
    }
}

void SceneManager::RenderCurrentScene() {
    if (m_currentScene) {
        m_currentScene->Render();
    }
}