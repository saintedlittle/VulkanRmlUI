#pragma once

#include "../Engine/Engine.h"
#include "../Core/EngineConfig.h"
#include <string>

class SettingsManager;

class AudioManager : public IEngineModule {
public:
    AudioManager(SettingsManager* settingsManager);
    ~AudioManager();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "AudioManager"; }
    int GetInitializationOrder() const override { return 350; }

    // Audio settings application
    void ApplyAudioSettings(const EngineConfig::Audio& audio);
    void OnSettingsChanged(const std::string& settingName);

private:
    SettingsManager* m_settingsManager;
    bool m_initialized = false;
    
    // Audio state
    float m_masterVolume = 1.0f;
    float m_musicVolume = 0.8f;
    float m_sfxVolume = 1.0f;
    std::string m_audioDevice = "default";
};