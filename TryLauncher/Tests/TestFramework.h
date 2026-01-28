#pragma once

// Engine includes for testing
#include "../Engine/Engine.h"
#include <memory>

// Test utilities (without external test frameworks)
namespace TestUtils {
    
    // Test helper functions
    bool IsValidEngineConfig(const EngineConfig& config);
    EngineConfig CreateMinimalValidConfig();
    EngineConfig CreateTestConfig();
}

// Simple test fixture for engine tests (without gtest dependency)
class EngineTestFixture {
public:
    void SetUp();
    void TearDown();
    
protected:
    std::unique_ptr<Engine> m_engine;
    EngineConfig m_testConfig;
};

// Simple property test base class (without external dependencies)
class PropertyTestBase {
public:
    void SetUp();
    void TearDown();
    
    // Common property test utilities
    bool ValidateEngineInitialization(const EngineConfig& config);
    bool ValidateEngineShutdown(Engine* engine);
    bool ValidateModuleLifecycle(IEngineModule* module);
};