#pragma once

#include "../Engine/Engine.h"
#include <string>
#include <functional>
#include "EventSystem.h"

class EventSystem;

struct SettingsChangedEvent : public Event {
    std::string settingName;
    std::string oldValue;
    std::string newValue;
    
    SettingsChangedEvent(const std::string& name, const std::string& oldVal, const std::string& newVal)
        : settingName(name), oldValue(oldVal), newValue(newVal) {}
    
    std::string GetType() const override { return "SettingsChangedEvent"; }
};

class SettingsManager : public IEngineModule {
public:
    SettingsManager(EventSystem* eventSystem);
    ~SettingsManager();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "SettingsManager"; }
    int GetInitializationOrder() const override { return 200; }

    // Settings management (to be implemented in later tasks)
    bool LoadSettings(const std::string& configPath);
    bool SaveSettings(const std::string& configPath);
    
    // Configuration access
    const EngineConfig& GetConfig() const { return m_config; }
    void SetConfig(const EngineConfig& config);
    
    // Individual setting access
    template<typename T>
    T GetSetting(const std::string& key, const T& defaultValue) const;
    
    template<typename T>
    void SetSetting(const std::string& key, const T& value);

private:
    EventSystem* m_eventSystem;
    EngineConfig m_config;
    std::string m_configPath;
    bool m_initialized = false;
};