#pragma once

#include "../Engine/Engine.h"
#include "../Core/EngineConfig.h"
#include <GLFW/glfw3.h>

class EventSystem;

/**
 * InputManager handles GLFW input capture and routes events to the event system.
 * This class:
 * - Captures keyboard and mouse events from GLFW callbacks
 * - Routes input events to RmlUI system and custom handlers
 * - Implements frame-based event processing coordination
 */
class InputManager : public IEngineModule {
public:
    InputManager(EventSystem* eventSystem);
    ~InputManager();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "InputManager"; }
    int GetInitializationOrder() const override { return 200; }

    // GLFW window setup
    void SetWindow(GLFWwindow* window);

    // Input state queries
    bool IsKeyPressed(int key) const;
    bool IsMouseButtonPressed(int button) const;
    void GetMousePosition(double& x, double& y) const;
    
    // Settings application
    void ApplyInputSettings(const EngineConfig::Input& input);
    void OnSettingsChanged(const std::string& settingName);

private:
    // GLFW callback functions
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void CursorPositionCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void CharCallback(GLFWwindow* window, unsigned int codepoint);
    static void WindowSizeCallback(GLFWwindow* window, int width, int height);
    static void WindowCloseCallback(GLFWwindow* window);

    // Internal event handling
    void HandleKeyEvent(int key, int scancode, int action, int mods);
    void HandleMouseButtonEvent(int button, int action, int mods);
    void HandleMouseMoveEvent(double xpos, double ypos);
    void HandleScrollEvent(double xoffset, double yoffset);
    void HandleCharEvent(unsigned int codepoint);
    void HandleWindowResizeEvent(int width, int height);
    void HandleWindowCloseEvent();

    EventSystem* m_eventSystem;
    GLFWwindow* m_window = nullptr;
    
    // Mouse state tracking
    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_firstMouseMove = true;
    
    bool m_initialized = false;
    
    // Static instance for callbacks
    static InputManager* s_instance;
};