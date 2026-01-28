#pragma once

#include "../Engine/Engine.h"
#include <stack>
#include <unordered_map>
#include <string>
#include <memory>
#include "EventSystem.h"

class SceneManager;
class EventSystem;

struct NavigationEvent : public Event {
    std::string targetScene;
    std::unordered_map<std::string, std::string> parameters;
    
    NavigationEvent(const std::string& target, const std::unordered_map<std::string, std::string>& params = {})
        : targetScene(target), parameters(params) {}
    
    std::string GetType() const override { return "NavigationEvent"; }
};

class TransitionEffect {
public:
    virtual ~TransitionEffect() = default;
    virtual void Start() = 0;
    virtual bool Update(float deltaTime) = 0; // Returns true when complete
    virtual void Render() = 0;
};

class NavigationManager : public IEngineModule {
public:
    NavigationManager(SceneManager* sceneManager, EventSystem* eventSystem);
    ~NavigationManager();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "NavigationManager"; }
    int GetInitializationOrder() const override { return 700; }

    // Navigation methods
    void NavigateTo(const std::string& sceneName, 
                   const std::unordered_map<std::string, std::string>& params = {});
    void NavigateBack();
    void SetTransitionEffect(std::unique_ptr<TransitionEffect> effect);
    
    // Navigation state
    bool CanNavigateBack() const { return m_navigationStack.size() > 1; }
    const std::string& GetCurrentScene() const;

private:
    void OnNavigationEvent(const NavigationEvent& event);
    
    SceneManager* m_sceneManager;
    EventSystem* m_eventSystem;
    
    std::stack<std::string> m_navigationStack;
    std::unique_ptr<TransitionEffect> m_transitionEffect;
    bool m_initialized = false;
};