#include "TestFramework.h"
#include <iostream>

namespace TestUtils {
    
    bool IsValidEngineConfig(const EngineConfig& config) {
        // Validate graphics settings
        if (config.graphics.windowWidth < 640 || config.graphics.windowWidth > 7680) return false;
        if (config.graphics.windowHeight < 480 || config.graphics.windowHeight > 4320) return false;
        if (config.graphics.msaaSamples != 1 && config.graphics.msaaSamples != 2 && 
            config.graphics.msaaSamples != 4 && config.graphics.msaaSamples != 8) return false;
        
        // Validate audio settings
        if (config.audio.masterVolume < 0.0f || config.audio.masterVolume > 1.0f) return false;
        if (config.audio.musicVolume < 0.0f || config.audio.musicVolume > 1.0f) return false;
        if (config.audio.sfxVolume < 0.0f || config.audio.sfxVolume > 1.0f) return false;
        
        // Validate input settings
        if (config.input.mouseSensitivity <= 0.0f || config.input.mouseSensitivity > 10.0f) return false;
        
        // Validate paths
        if (config.assetPath.empty() || config.configPath.empty()) return false;
        
        return true;
    }
    
    EngineConfig CreateMinimalValidConfig() {
        EngineConfig config;
        config.graphics.windowWidth = 800;
        config.graphics.windowHeight = 600;
        config.graphics.fullscreen = false;
        config.graphics.vsync = true;
        config.graphics.msaaSamples = 1;
        config.graphics.enableValidation = false;
        
        config.audio.masterVolume = 1.0f;
        config.audio.musicVolume = 0.8f;
        config.audio.sfxVolume = 1.0f;
        config.audio.audioDevice = "default";
        
        config.input.mouseSensitivity = 1.0f;
        
        config.assetPath = "assets/";
        config.configPath = "config.json";
        
        return config;
    }
    
    EngineConfig CreateTestConfig() {
        EngineConfig config = CreateMinimalValidConfig();
        config.graphics.enableValidation = true; // Enable validation for testing
        return config;
    }
}

// EngineTestFixture implementation
void EngineTestFixture::SetUp() {
    m_testConfig = TestUtils::CreateTestConfig();
    m_engine = std::make_unique<Engine>();
}

void EngineTestFixture::TearDown() {
    if (m_engine) {
        m_engine->Shutdown();
        m_engine.reset();
    }
}

// PropertyTestBase implementation
void PropertyTestBase::SetUp() {
    // Basic setup for property tests
}

void PropertyTestBase::TearDown() {
    // Clean up after property tests
}

bool PropertyTestBase::ValidateEngineInitialization(const EngineConfig& config) {
    if (!TestUtils::IsValidEngineConfig(config)) {
        return false;
    }
    
    Engine engine;
    bool initResult = engine.Initialize(config);
    
    if (initResult) {
        // Verify engine is in expected state
        bool isRunning = engine.IsRunning();
        engine.Shutdown();
        return isRunning;
    }
    
    return false;
}

bool PropertyTestBase::ValidateEngineShutdown(Engine* engine) {
    if (!engine) return false;
    
    engine->Shutdown();
    return !engine->IsRunning();
}

bool PropertyTestBase::ValidateModuleLifecycle(IEngineModule* module) {
    if (!module) return false;
    
    // Test initialization
    bool initResult = module->Initialize();
    if (!initResult) return false;
    
    // Test update (should not crash)
    try {
        module->Update(0.016f); // 60 FPS delta
    } catch (...) {
        return false;
    }
    
    // Test shutdown
    try {
        module->Shutdown();
    } catch (...) {
        return false;
    }
    
    return true;
}