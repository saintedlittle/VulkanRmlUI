#include "SettingsManager.h"
#include "EventSystem.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <GLFW/glfw3.h>

SettingsManager::SettingsManager(EventSystem* eventSystem)
    : m_eventSystem(eventSystem), m_configPath("config.txt") {
}

SettingsManager::~SettingsManager() {
    if (m_initialized) {
        Shutdown();
    }
}

bool SettingsManager::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing SettingsManager..." << std::endl;
    
    // Load default configuration
    m_config = GetDefaultConfig();
    
    // Try to load settings from file
    if (std::filesystem::exists(m_configPath)) {
        if (LoadSettings(m_configPath)) {
            std::cout << "Loaded settings from: " << m_configPath << std::endl;
        } else {
            std::cout << "Failed to load settings, using defaults" << std::endl;
        }
    } else {
        std::cout << "No config file found, using default settings" << std::endl;
        // Save default configuration
        SaveSettings(m_configPath);
    }
    
    // Apply default key bindings if none exist
    if (m_config.input.keyBindings.empty()) {
        ApplyDefaultKeyBindings();
    }
    
    m_initialized = true;
    std::cout << "SettingsManager initialized successfully" << std::endl;
    return true;
}

void SettingsManager::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }
    
    // Settings manager doesn't need regular updates
    // All changes are handled immediately when settings are modified
}

void SettingsManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down SettingsManager..." << std::endl;
    
    // Save current settings
    if (!SaveSettings(m_configPath)) {
        std::cerr << "Failed to save settings on shutdown" << std::endl;
    }
    
    // Clear callbacks
    m_changeCallbacks.clear();
    
    m_initialized = false;
    std::cout << "SettingsManager shutdown complete" << std::endl;
}

bool SettingsManager::LoadSettings(const std::string& configPath) {
    std::string path = configPath.empty() ? m_configPath : configPath;
    return LoadFromTextFile(path);
}

bool SettingsManager::SaveSettings(const std::string& configPath) const {
    std::string path = configPath.empty() ? m_configPath : configPath;
    return SaveToTextFile(path);
}

void SettingsManager::SetConfig(const EngineConfig& config) {
    if (!ValidateConfig(config)) {
        std::cerr << "Invalid configuration provided to SetConfig" << std::endl;
        return;
    }
    
    EngineConfig oldConfig = m_config;
    m_config = config;
    
    // Save immediately
    SaveSettings();
    
    // Notify event system of configuration change
    if (m_eventSystem) {
        auto event = std::make_unique<SettingsChangedEvent>("config", "old", "new");
        m_eventSystem->PublishEvent(std::move(event));
    }
    
    std::cout << "Configuration updated and saved" << std::endl;
}

template<typename T>
T SettingsManager::GetSetting(const std::string& key, const T& defaultValue) const {
    // Graphics settings
    if constexpr (std::is_same_v<T, uint32_t> || std::is_same_v<T, int>) {
        if (key == "graphics.windowWidth") return static_cast<T>(m_config.graphics.windowWidth);
        if (key == "graphics.windowHeight") return static_cast<T>(m_config.graphics.windowHeight);
        if (key == "graphics.msaaSamples") return static_cast<T>(m_config.graphics.msaaSamples);
    }
    
    if constexpr (std::is_same_v<T, bool>) {
        if (key == "graphics.fullscreen") return static_cast<T>(m_config.graphics.fullscreen);
        if (key == "graphics.vsync") return static_cast<T>(m_config.graphics.vsync);
        if (key == "graphics.enableValidation") return static_cast<T>(m_config.graphics.enableValidation);
    }
    
    // Audio settings
    if constexpr (std::is_same_v<T, float>) {
        if (key == "audio.masterVolume") return static_cast<T>(m_config.audio.masterVolume);
        if (key == "audio.musicVolume") return static_cast<T>(m_config.audio.musicVolume);
        if (key == "audio.sfxVolume") return static_cast<T>(m_config.audio.sfxVolume);
        if (key == "input.mouseSensitivity") return static_cast<T>(m_config.input.mouseSensitivity);
    }
    
    // String settings
    if constexpr (std::is_same_v<T, std::string>) {
        if (key == "graphics.preferredGPU") return m_config.graphics.preferredGPU;
        if (key == "audio.audioDevice") return m_config.audio.audioDevice;
        if (key == "assetPath") return m_config.assetPath;
        if (key == "configPath") return m_config.configPath;
    }
    
    std::cerr << "Unknown setting key: " << key << std::endl;
    return defaultValue;
}

template<typename T>
bool SettingsManager::SetSetting(const std::string& key, const T& value) {
    SettingValue oldValue, newValue;
    bool changed = false;
    
    // Graphics settings - integers
    if constexpr (std::is_integral_v<T> && !std::is_same_v<T, bool>) {
        if (key == "graphics.windowWidth") {
            uint32_t newVal = static_cast<uint32_t>(value);
            if (newVal >= EngineConfig::Graphics::MIN_WIDTH && newVal <= EngineConfig::Graphics::MAX_WIDTH) {
                oldValue = static_cast<int>(m_config.graphics.windowWidth);
                m_config.graphics.windowWidth = newVal;
                newValue = static_cast<int>(newVal);
                changed = true;
            }
        }
        else if (key == "graphics.windowHeight") {
            uint32_t newVal = static_cast<uint32_t>(value);
            if (newVal >= EngineConfig::Graphics::MIN_HEIGHT && newVal <= EngineConfig::Graphics::MAX_HEIGHT) {
                oldValue = static_cast<int>(m_config.graphics.windowHeight);
                m_config.graphics.windowHeight = newVal;
                newValue = static_cast<int>(newVal);
                changed = true;
            }
        }
        else if (key == "graphics.msaaSamples") {
            uint32_t newVal = static_cast<uint32_t>(value);
            if (newVal == 1 || newVal == 2 || newVal == 4 || newVal == 8 || newVal == 16) {
                oldValue = static_cast<int>(m_config.graphics.msaaSamples);
                m_config.graphics.msaaSamples = newVal;
                newValue = static_cast<int>(newVal);
                changed = true;
            }
        }
    }
    
    // Graphics settings - booleans
    if constexpr (std::is_same_v<T, bool>) {
        if (key == "graphics.fullscreen") {
            oldValue = m_config.graphics.fullscreen;
            m_config.graphics.fullscreen = value;
            newValue = value;
            changed = true;
        }
        else if (key == "graphics.vsync") {
            oldValue = m_config.graphics.vsync;
            m_config.graphics.vsync = value;
            newValue = value;
            changed = true;
        }
        else if (key == "graphics.enableValidation") {
            oldValue = m_config.graphics.enableValidation;
            m_config.graphics.enableValidation = value;
            newValue = value;
            changed = true;
        }
    }
    
    // Audio settings - floats
    if constexpr (std::is_floating_point_v<T>) {
        if (key == "audio.masterVolume") {
            float newVal = static_cast<float>(value);
            if (newVal >= EngineConfig::Audio::MIN_VOLUME && newVal <= EngineConfig::Audio::MAX_VOLUME) {
                oldValue = m_config.audio.masterVolume;
                m_config.audio.masterVolume = newVal;
                newValue = newVal;
                changed = true;
            }
        }
        else if (key == "audio.musicVolume") {
            float newVal = static_cast<float>(value);
            if (newVal >= EngineConfig::Audio::MIN_VOLUME && newVal <= EngineConfig::Audio::MAX_VOLUME) {
                oldValue = m_config.audio.musicVolume;
                m_config.audio.musicVolume = newVal;
                newValue = newVal;
                changed = true;
            }
        }
        else if (key == "audio.sfxVolume") {
            float newVal = static_cast<float>(value);
            if (newVal >= EngineConfig::Audio::MIN_VOLUME && newVal <= EngineConfig::Audio::MAX_VOLUME) {
                oldValue = m_config.audio.sfxVolume;
                m_config.audio.sfxVolume = newVal;
                newValue = newVal;
                changed = true;
            }
        }
        else if (key == "input.mouseSensitivity") {
            float newVal = static_cast<float>(value);
            if (newVal >= EngineConfig::Input::MIN_SENSITIVITY && newVal <= EngineConfig::Input::MAX_SENSITIVITY) {
                oldValue = m_config.input.mouseSensitivity;
                m_config.input.mouseSensitivity = newVal;
                newValue = newVal;
                changed = true;
            }
        }
    }
    
    // String settings
    if constexpr (std::is_same_v<T, std::string>) {
        if (key == "graphics.preferredGPU") {
            oldValue = m_config.graphics.preferredGPU;
            m_config.graphics.preferredGPU = value;
            newValue = value;
            changed = true;
        }
        else if (key == "audio.audioDevice") {
            oldValue = m_config.audio.audioDevice;
            m_config.audio.audioDevice = value;
            newValue = value;
            changed = true;
        }
        else if (key == "assetPath") {
            oldValue = m_config.assetPath;
            m_config.assetPath = value;
            newValue = value;
            changed = true;
        }
        else if (key == "configPath") {
            oldValue = m_config.configPath;
            m_config.configPath = value;
            newValue = value;
            changed = true;
        }
    }
    
    if (changed) {
        // Save immediately
        SaveSettings();
        
        // Notify of change
        NotifySettingChanged(key, oldValue, newValue);
        
        std::cout << "Setting updated: " << key << std::endl;
        return true;
    } else {
        std::cerr << "Failed to set setting: " << key << " (invalid value or type)" << std::endl;
        return false;
    }
}

bool SettingsManager::ValidateSetting(const std::string& key, const SettingValue& value) const {
    // This is a simplified validation - in a real implementation,
    // you'd have more comprehensive validation logic
    return true;
}

void SettingsManager::RegisterChangeCallback(const std::string& settingName, SettingsChangeCallback callback) {
    m_changeCallbacks[settingName] = callback;
}

void SettingsManager::UnregisterChangeCallback(const std::string& settingName) {
    m_changeCallbacks.erase(settingName);
}

EngineConfig SettingsManager::GetDefaultConfig() {
    EngineConfig config;
    
    // Graphics defaults are already set in the struct
    // Audio defaults are already set in the struct
    // Input defaults will be set by ApplyDefaultKeyBindings
    
    return config;
}

void SettingsManager::NotifySettingChanged(const std::string& key, const SettingValue& oldValue, const SettingValue& newValue) {
    // Call registered callback if exists
    auto it = m_changeCallbacks.find(key);
    if (it != m_changeCallbacks.end()) {
        it->second(key, newValue);
    }
    
    // Publish event to event system
    if (m_eventSystem) {
        std::string oldStr = std::visit([](const auto& v) -> std::string {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
                return v ? "true" : "false";
            } else {
                return std::to_string(v);
            }
        }, oldValue);
        
        std::string newStr = std::visit([](const auto& v) -> std::string {
            if constexpr (std::is_same_v<std::decay_t<decltype(v)>, std::string>) {
                return v;
            } else if constexpr (std::is_same_v<std::decay_t<decltype(v)>, bool>) {
                return v ? "true" : "false";
            } else {
                return std::to_string(v);
            }
        }, newValue);
        
        auto event = std::make_unique<SettingsChangedEvent>(key, oldStr, newStr);
        m_eventSystem->PublishEvent(std::move(event));
    }
}

bool SettingsManager::ValidateConfig(const EngineConfig& config) const {
    return config.IsValid();
}

void SettingsManager::ApplyDefaultKeyBindings() {
    // Set up default key bindings
    m_config.input.keyBindings["move_forward"] = GLFW_KEY_W;
    m_config.input.keyBindings["move_backward"] = GLFW_KEY_S;
    m_config.input.keyBindings["move_left"] = GLFW_KEY_A;
    m_config.input.keyBindings["move_right"] = GLFW_KEY_D;
    m_config.input.keyBindings["jump"] = GLFW_KEY_SPACE;
    m_config.input.keyBindings["crouch"] = GLFW_KEY_LEFT_CONTROL;
    m_config.input.keyBindings["run"] = GLFW_KEY_LEFT_SHIFT;
    m_config.input.keyBindings["interact"] = GLFW_KEY_E;
    m_config.input.keyBindings["menu"] = GLFW_KEY_ESCAPE;
    m_config.input.keyBindings["inventory"] = GLFW_KEY_TAB;
    
    std::cout << "Applied default key bindings" << std::endl;
}

bool SettingsManager::SaveToTextFile(const std::string& path) const {
    try {
        // Create directory if it doesn't exist
        std::filesystem::path filePath(path);
        if (filePath.has_parent_path()) {
            std::filesystem::create_directories(filePath.parent_path());
        }
        
        std::ofstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to create config file: " << path << std::endl;
            return false;
        }
        
        // Write configuration in simple key=value format
        file << "# TryLauncher Configuration File\n";
        file << "# Graphics Settings\n";
        file << "graphics.windowWidth=" << m_config.graphics.windowWidth << "\n";
        file << "graphics.windowHeight=" << m_config.graphics.windowHeight << "\n";
        file << "graphics.fullscreen=" << (m_config.graphics.fullscreen ? "true" : "false") << "\n";
        file << "graphics.vsync=" << (m_config.graphics.vsync ? "true" : "false") << "\n";
        file << "graphics.msaaSamples=" << m_config.graphics.msaaSamples << "\n";
        file << "graphics.enableValidation=" << (m_config.graphics.enableValidation ? "true" : "false") << "\n";
        file << "graphics.preferredGPU=" << m_config.graphics.preferredGPU << "\n";
        
        file << "# Audio Settings\n";
        file << "audio.masterVolume=" << m_config.audio.masterVolume << "\n";
        file << "audio.musicVolume=" << m_config.audio.musicVolume << "\n";
        file << "audio.sfxVolume=" << m_config.audio.sfxVolume << "\n";
        file << "audio.audioDevice=" << m_config.audio.audioDevice << "\n";
        
        file << "# Input Settings\n";
        file << "input.mouseSensitivity=" << m_config.input.mouseSensitivity << "\n";
        
        // Write key bindings
        for (const auto& binding : m_config.input.keyBindings) {
            file << "input.keyBinding." << binding.first << "=" << binding.second << "\n";
        }
        
        file << "# General Settings\n";
        file << "assetPath=" << m_config.assetPath << "\n";
        file << "configPath=" << m_config.configPath << "\n";
        
        std::cout << "Successfully saved settings to: " << path << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error saving settings: " << e.what() << std::endl;
        return false;
    }
}

bool SettingsManager::LoadFromTextFile(const std::string& path) {
    try {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open config file: " << path << std::endl;
            return false;
        }
        
        EngineConfig newConfig = GetDefaultConfig();
        std::string line;
        
        while (std::getline(file, line)) {
            // Skip comments and empty lines
            if (line.empty() || line[0] == '#') {
                continue;
            }
            
            // Parse key=value pairs
            size_t equalPos = line.find('=');
            if (equalPos == std::string::npos) {
                continue;
            }
            
            std::string key = line.substr(0, equalPos);
            std::string value = line.substr(equalPos + 1);
            
            // Parse different setting types
            if (key == "graphics.windowWidth") {
                newConfig.graphics.windowWidth = std::stoul(value);
            } else if (key == "graphics.windowHeight") {
                newConfig.graphics.windowHeight = std::stoul(value);
            } else if (key == "graphics.fullscreen") {
                newConfig.graphics.fullscreen = (value == "true");
            } else if (key == "graphics.vsync") {
                newConfig.graphics.vsync = (value == "true");
            } else if (key == "graphics.msaaSamples") {
                newConfig.graphics.msaaSamples = std::stoul(value);
            } else if (key == "graphics.enableValidation") {
                newConfig.graphics.enableValidation = (value == "true");
            } else if (key == "graphics.preferredGPU") {
                newConfig.graphics.preferredGPU = value;
            } else if (key == "audio.masterVolume") {
                newConfig.audio.masterVolume = std::stof(value);
            } else if (key == "audio.musicVolume") {
                newConfig.audio.musicVolume = std::stof(value);
            } else if (key == "audio.sfxVolume") {
                newConfig.audio.sfxVolume = std::stof(value);
            } else if (key == "audio.audioDevice") {
                newConfig.audio.audioDevice = value;
            } else if (key == "input.mouseSensitivity") {
                newConfig.input.mouseSensitivity = std::stof(value);
            } else if (key.substr(0, 18) == "input.keyBinding.") {
                std::string bindingName = key.substr(18);
                newConfig.input.keyBindings[bindingName] = std::stoi(value);
            } else if (key == "assetPath") {
                newConfig.assetPath = value;
            } else if (key == "configPath") {
                newConfig.configPath = value;
            }
        }
        
        // Validate loaded configuration
        if (!ValidateConfig(newConfig)) {
            std::cerr << "Invalid configuration in file: " << path << std::endl;
            return false;
        }
        
        m_config = newConfig;
        std::cout << "Successfully loaded settings from: " << path << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Error loading settings: " << e.what() << std::endl;
        return false;
    }
}

// Explicit template instantiations for common types
template bool SettingsManager::GetSetting<bool>(const std::string&, const bool&) const;
template int SettingsManager::GetSetting<int>(const std::string&, const int&) const;
template uint32_t SettingsManager::GetSetting<uint32_t>(const std::string&, const uint32_t&) const;
template float SettingsManager::GetSetting<float>(const std::string&, const float&) const;
template std::string SettingsManager::GetSetting<std::string>(const std::string&, const std::string&) const;

template bool SettingsManager::SetSetting<bool>(const std::string&, const bool&);
template bool SettingsManager::SetSetting<int>(const std::string&, const int&);
template bool SettingsManager::SetSetting<uint32_t>(const std::string&, const uint32_t&);
template bool SettingsManager::SetSetting<float>(const std::string&, const float&);
template bool SettingsManager::SetSetting<std::string>(const std::string&, const std::string&);