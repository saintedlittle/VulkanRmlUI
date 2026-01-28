#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>

// Enhanced EngineConfig structure with validation
struct EngineConfig {
    struct Graphics {
        uint32_t windowWidth = 1920;
        uint32_t windowHeight = 1080;
        bool fullscreen = false;
        bool vsync = true;
        uint32_t msaaSamples = 1;
        bool enableValidation = false;
        std::string preferredGPU = "auto";
        
        // Validation ranges
        static constexpr uint32_t MIN_WIDTH = 800;
        static constexpr uint32_t MAX_WIDTH = 7680;
        static constexpr uint32_t MIN_HEIGHT = 600;
        static constexpr uint32_t MAX_HEIGHT = 4320;
        static constexpr uint32_t MAX_MSAA_SAMPLES = 16;
        
        bool IsValid() const {
            return windowWidth >= MIN_WIDTH && windowWidth <= MAX_WIDTH &&
                   windowHeight >= MIN_HEIGHT && windowHeight <= MAX_HEIGHT &&
                   (msaaSamples == 1 || msaaSamples == 2 || msaaSamples == 4 || 
                    msaaSamples == 8 || msaaSamples == 16);
        }
    } graphics;
    
    struct Audio {
        float masterVolume = 1.0f;
        float musicVolume = 0.8f;
        float sfxVolume = 1.0f;
        std::string audioDevice = "default";
        
        // Validation ranges
        static constexpr float MIN_VOLUME = 0.0f;
        static constexpr float MAX_VOLUME = 1.0f;
        
        bool IsValid() const {
            return masterVolume >= MIN_VOLUME && masterVolume <= MAX_VOLUME &&
                   musicVolume >= MIN_VOLUME && musicVolume <= MAX_VOLUME &&
                   sfxVolume >= MIN_VOLUME && sfxVolume <= MAX_VOLUME;
        }
    } audio;
    
    struct Input {
        std::unordered_map<std::string, int> keyBindings;
        float mouseSensitivity = 1.0f;
        
        // Validation ranges
        static constexpr float MIN_SENSITIVITY = 0.1f;
        static constexpr float MAX_SENSITIVITY = 5.0f;
        
        bool IsValid() const {
            return mouseSensitivity >= MIN_SENSITIVITY && mouseSensitivity <= MAX_SENSITIVITY;
        }
    } input;
    
    std::string assetPath = "assets/";
    std::string configPath = "config.json";
    
    bool IsValid() const {
        return graphics.IsValid() && audio.IsValid() && input.IsValid();
    }
};