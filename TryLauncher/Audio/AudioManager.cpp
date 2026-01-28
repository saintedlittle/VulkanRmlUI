#include "AudioManager.h"
#include "../Core/SettingsManager.h"
#include "../Core/EngineConfig.h"
#include <iostream>

AudioManager::AudioManager(SettingsManager* settingsManager)
    : m_settingsManager(settingsManager) {
}

AudioManager::~AudioManager() {
    if (m_initialized) {
        Shutdown();
    }
}

bool AudioManager::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing AudioManager..." << std::endl;
    
    // Apply current audio settings
    if (m_settingsManager) {
        const auto& config = m_settingsManager->GetConfig();
        ApplyAudioSettings(config.audio);
    }
    
    m_initialized = true;
    std::cout << "AudioManager initialized successfully" << std::endl;
    return true;
}

void AudioManager::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }
    
    // Audio system updates would go here
    // For now, this is a placeholder
}

void AudioManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down AudioManager..." << std::endl;
    
    // Cleanup audio resources
    
    m_initialized = false;
    std::cout << "AudioManager shutdown complete" << std::endl;
}

void AudioManager::ApplyAudioSettings(const EngineConfig::Audio& audio) {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Applying audio settings..." << std::endl;
    
    // Apply volume settings
    if (m_masterVolume != audio.masterVolume) {
        m_masterVolume = audio.masterVolume;
        std::cout << "Master volume set to: " << m_masterVolume << std::endl;
        // In a real implementation, this would update the audio system
    }
    
    if (m_musicVolume != audio.musicVolume) {
        m_musicVolume = audio.musicVolume;
        std::cout << "Music volume set to: " << m_musicVolume << std::endl;
        // In a real implementation, this would update music channels
    }
    
    if (m_sfxVolume != audio.sfxVolume) {
        m_sfxVolume = audio.sfxVolume;
        std::cout << "SFX volume set to: " << m_sfxVolume << std::endl;
        // In a real implementation, this would update SFX channels
    }
    
    // Apply audio device setting
    if (m_audioDevice != audio.audioDevice) {
        m_audioDevice = audio.audioDevice;
        std::cout << "Audio device set to: " << m_audioDevice << std::endl;
        // In a real implementation, this would switch audio devices
    }
    
    std::cout << "Audio settings applied successfully" << std::endl;
}

void AudioManager::OnSettingsChanged(const std::string& settingName) {
    if (!m_initialized || !m_settingsManager) {
        return;
    }
    
    // Handle audio-related setting changes
    if (settingName.find("audio.") == 0) {
        const auto& config = m_settingsManager->GetConfig();
        ApplyAudioSettings(config.audio);
    }
}