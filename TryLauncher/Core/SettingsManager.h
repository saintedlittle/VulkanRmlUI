#pragma once

#include "../Engine/Engine.h"
#include "EngineConfig.h"
#include <string>
#include <functional>
#include <unordered_map>
#include <variant>
#include <nlohmann/json.hpp>
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
    using SettingValue = std::variant<bool, int, float, std::string>;
    using SettingsChangeCallback = std::function<void(const std::string&, const SettingValue&)>;

    SettingsManager(EventSystem* eventSystem);
    ~SettingsManager();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "SettingsManager"; }
    int GetInitializationOrder() const override { return 200; }

    // Settings persistence (simplified for now)
    bool LoadSettings(const std::string& configPath = "");
    bool SaveSettings(const std::string& configPath = "") const;
    
    // Configuration access
    const EngineConfig& GetConfig() const { return m_config; }
    void SetConfig(const EngineConfig& config);
    
    // Individual setting access with validation
    template<typename T>
    T GetSetting(const std::string& key, const T& defaultValue) const;
    
    template<typename T>
    bool SetSetting(const std::string& key, const T& value);
    
    // Setting validation
    bool ValidateSetting(const std::string& key, const SettingValue& value) const;
    
    // Change notifications
    void RegisterChangeCallback(const std::string& settingName, SettingsChangeCallback callback);
    void UnregisterChangeCallback(const std::string& settingName);
    
    // Default configuration
    static EngineConfig GetDefaultConfig();

private:
    EventSystem* m_eventSystem;
    EngineConfig m_config;
    std::string m_configPath;
    bool m_initialized = false;
    
    // Change callbacks
    std::unordered_map<std::string, SettingsChangeCallback> m_changeCallbacks;
    
    // Helper methods
    void NotifySettingChanged(const std::string& key, const SettingValue& oldValue, const SettingValue& newValue);
    bool ValidateConfig(const EngineConfig& config) const;
    void ApplyDefaultKeyBindings();
    
    // Simple text-based serialization (will be replaced with JSON later)
    bool SaveToTextFile(const std::string& path) const;
    bool LoadFromTextFile(const std::string& path);
};