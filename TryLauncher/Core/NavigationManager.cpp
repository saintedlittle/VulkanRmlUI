#include "NavigationManager.h"
#include "SceneManager.h"
#include "EventSystem.h"
#include <iostream>

NavigationManager::NavigationManager(SceneManager* sceneManager, EventSystem* eventSystem)
    : m_sceneManager(sceneManager), m_eventSystem(eventSystem) {
}

NavigationManager::~NavigationManager() {
    if (m_initialized) {
        Shutdown();
    }
}

bool NavigationManager::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing NavigationManager..." << std::endl;
    
    // Subscribe to navigation events
    m_eventSystem->Subscribe<NavigationEvent>(
        [this](const NavigationEvent& event) {
            OnNavigationEvent(event);
        });
    
    m_initialized = true;
    std::cout << "NavigationManager initialized" << std::endl;
    return true;
}

void NavigationManager::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }
    
    // Update transition effect if active
    if (m_transitionEffect) {
        if (m_transitionEffect->Update(deltaTime)) {
            // Transition complete
            m_transitionEffect.reset();
        }
    }
}

void NavigationManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down NavigationManager..." << std::endl;
    
    // Clear navigation stack
    while (!m_navigationStack.empty()) {
        m_navigationStack.pop();
    }
    
    // Clear transition effect
    m_transitionEffect.reset();
    
    m_initialized = false;
    std::cout << "NavigationManager shutdown complete" << std::endl;
}

void NavigationManager::NavigateTo(const std::string& sceneName, 
                                  const std::unordered_map<std::string, std::string>& params) {
    if (!m_initialized) {
        std::cerr << "NavigationManager not initialized!" << std::endl;
        return;
    }
    
    // Start transition effect if available
    if (m_transitionEffect) {
        m_transitionEffect->Start();
    }
    
    // Switch to the new scene
    if (m_sceneManager->SwitchToScene(sceneName)) {
        // Add to navigation stack
        m_navigationStack.push(sceneName);
        std::cout << "Navigated to scene: " << sceneName << std::endl;
    } else {
        std::cerr << "Failed to navigate to scene: " << sceneName << std::endl;
    }
}

void NavigationManager::NavigateBack() {
    if (!CanNavigateBack()) {
        std::cout << "Cannot navigate back - no previous scene" << std::endl;
        return;
    }
    
    // Remove current scene from stack
    m_navigationStack.pop();
    
    // Navigate to previous scene
    if (!m_navigationStack.empty()) {
        const std::string& previousScene = m_navigationStack.top();
        
        // Start transition effect if available
        if (m_transitionEffect) {
            m_transitionEffect->Start();
        }
        
        if (m_sceneManager->SwitchToScene(previousScene)) {
            std::cout << "Navigated back to scene: " << previousScene << std::endl;
        } else {
            std::cerr << "Failed to navigate back to scene: " << previousScene << std::endl;
        }
    }
}

void NavigationManager::SetTransitionEffect(std::unique_ptr<TransitionEffect> effect) {
    m_transitionEffect = std::move(effect);
}

const std::string& NavigationManager::GetCurrentScene() const {
    if (m_navigationStack.empty()) {
        static const std::string empty;
        return empty;
    }
    return m_navigationStack.top();
}

void NavigationManager::OnNavigationEvent(const NavigationEvent& event) {
    NavigateTo(event.targetScene, event.parameters);
}