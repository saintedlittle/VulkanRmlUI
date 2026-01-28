#pragma once

#include "../Engine/Engine.h"
#include <unordered_map>
#include <memory>
#include <string>

class EventSystem;
class RmlUISystem;

class Scene {
public:
    virtual ~Scene() = default;
    virtual bool Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void Cleanup() = 0;
    virtual void OnEnter() {}
    virtual void OnExit() {}
};

class SceneManager : public IEngineModule {
public:
    SceneManager(EventSystem* eventSystem, RmlUISystem* uiSystem);
    ~SceneManager();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "SceneManager"; }
    int GetInitializationOrder() const override { return 600; }

    // Scene management
    void RegisterScene(const std::string& name, std::unique_ptr<Scene> scene);
    bool SwitchToScene(const std::string& name);
    void UpdateCurrentScene(float deltaTime);
    void RenderCurrentScene();
    
    Scene* GetCurrentScene() const { return m_currentScene; }
    const std::string& GetCurrentSceneName() const { return m_currentSceneName; }

private:
    EventSystem* m_eventSystem;
    RmlUISystem* m_uiSystem;
    
    std::unordered_map<std::string, std::unique_ptr<Scene>> m_scenes;
    Scene* m_currentScene = nullptr;
    std::string m_currentSceneName;
    bool m_initialized = false;
};