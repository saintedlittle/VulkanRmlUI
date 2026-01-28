#include "Engine/Engine.h"
#include "Core/SettingsManager.h"
#include "Core/EngineConfig.h"
#include <iostream>

int main() {
    std::cout << "TryLauncher - Vulkan + RmlUI Game Engine" << std::endl;
    
    // Create engine with default configuration
    Engine engine;
    EngineConfig config = SettingsManager::GetDefaultConfig();
    
    // Initialize the engine
    if (!engine.Initialize(config)) {
        std::cerr << "Failed to initialize engine!" << std::endl;
        return -1;
    }
    
    std::cout << "Engine initialized successfully!" << std::endl;
    std::cout << "All modules loaded and ready." << std::endl;
    
    // For now, just test initialization and shutdown
    // The actual game loop will be implemented in later tasks
    std::cout << "Testing engine shutdown..." << std::endl;
    
    engine.Shutdown();
    
    std::cout << "Engine shutdown complete. Infrastructure test passed!" << std::endl;
    return 0;
}
