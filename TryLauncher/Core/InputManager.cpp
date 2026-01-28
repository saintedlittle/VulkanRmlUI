#include "InputManager.h"
#include "EventSystem.h"
#include "InputEvents.h"
#include <iostream>

// Static instance for callbacks
InputManager* InputManager::s_instance = nullptr;

InputManager::InputManager(EventSystem* eventSystem)
    : m_eventSystem(eventSystem) {
    s_instance = this;
}

InputManager::~InputManager() {
    if (m_initialized) {
        Shutdown();
    }
    
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

bool InputManager::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing InputManager..." << std::endl;
    
    if (!m_eventSystem) {
        std::cerr << "InputManager: EventSystem is required" << std::endl;
        return false;
    }
    
    m_initialized = true;
    std::cout << "InputManager initialized" << std::endl;
    return true;
}

void InputManager::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }
    
    // Poll GLFW events
    if (m_window) {
        glfwPollEvents();
    }
}

void InputManager::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down InputManager..." << std::endl;
    
    // Remove GLFW callbacks
    if (m_window) {
        glfwSetKeyCallback(m_window, nullptr);
        glfwSetMouseButtonCallback(m_window, nullptr);
        glfwSetCursorPosCallback(m_window, nullptr);
        glfwSetScrollCallback(m_window, nullptr);
        glfwSetCharCallback(m_window, nullptr);
        glfwSetWindowSizeCallback(m_window, nullptr);
        glfwSetWindowCloseCallback(m_window, nullptr);
    }
    
    m_window = nullptr;
    m_initialized = false;
    std::cout << "InputManager shutdown complete" << std::endl;
}

void InputManager::SetWindow(GLFWwindow* window) {
    m_window = window;
    
    if (m_window && m_initialized) {
        // Set up GLFW callbacks
        glfwSetKeyCallback(m_window, KeyCallback);
        glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
        glfwSetCursorPosCallback(m_window, CursorPositionCallback);
        glfwSetScrollCallback(m_window, ScrollCallback);
        glfwSetCharCallback(m_window, CharCallback);
        glfwSetWindowSizeCallback(m_window, WindowSizeCallback);
        glfwSetWindowCloseCallback(m_window, WindowCloseCallback);
        
        // Initialize mouse position
        double xpos, ypos;
        glfwGetCursorPos(m_window, &xpos, &ypos);
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        m_firstMouseMove = true;
        
        std::cout << "InputManager: GLFW callbacks set up for window" << std::endl;
    }
}

bool InputManager::IsKeyPressed(int key) const {
    if (!m_window) {
        return false;
    }
    
    return glfwGetKey(m_window, key) == GLFW_PRESS;
}

bool InputManager::IsMouseButtonPressed(int button) const {
    if (!m_window) {
        return false;
    }
    
    return glfwGetMouseButton(m_window, button) == GLFW_PRESS;
}

void InputManager::GetMousePosition(double& x, double& y) const {
    if (m_window) {
        glfwGetCursorPos(m_window, &x, &y);
    } else {
        x = 0.0;
        y = 0.0;
    }
}

// GLFW callback implementations
void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (s_instance) {
        s_instance->HandleKeyEvent(key, scancode, action, mods);
    }
}

void InputManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (s_instance) {
        s_instance->HandleMouseButtonEvent(button, action, mods);
    }
}

void InputManager::CursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    if (s_instance) {
        s_instance->HandleMouseMoveEvent(xpos, ypos);
    }
}

void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (s_instance) {
        s_instance->HandleScrollEvent(xoffset, yoffset);
    }
}

void InputManager::CharCallback(GLFWwindow* window, unsigned int codepoint) {
    if (s_instance) {
        s_instance->HandleCharEvent(codepoint);
    }
}

void InputManager::WindowSizeCallback(GLFWwindow* window, int width, int height) {
    if (s_instance) {
        s_instance->HandleWindowResizeEvent(width, height);
    }
}

void InputManager::WindowCloseCallback(GLFWwindow* window) {
    if (s_instance) {
        s_instance->HandleWindowCloseEvent();
    }
}

// Internal event handling
void InputManager::HandleKeyEvent(int key, int scancode, int action, int mods) {
    if (m_eventSystem) {
        KeyEvent event(key, scancode, action, mods);
        m_eventSystem->Publish(event);
    }
}

void InputManager::HandleMouseButtonEvent(int button, int action, int mods) {
    if (m_eventSystem) {
        double xpos, ypos;
        GetMousePosition(xpos, ypos);
        MouseButtonEvent event(button, action, mods, xpos, ypos);
        m_eventSystem->Publish(event);
    }
}

void InputManager::HandleMouseMoveEvent(double xpos, double ypos) {
    if (m_eventSystem) {
        double deltaX = 0.0;
        double deltaY = 0.0;
        
        if (!m_firstMouseMove) {
            deltaX = xpos - m_lastMouseX;
            deltaY = ypos - m_lastMouseY;
        } else {
            m_firstMouseMove = false;
        }
        
        m_lastMouseX = xpos;
        m_lastMouseY = ypos;
        
        MouseMoveEvent event(xpos, ypos, deltaX, deltaY);
        m_eventSystem->Publish(event);
    }
}

void InputManager::HandleScrollEvent(double xoffset, double yoffset) {
    if (m_eventSystem) {
        MouseScrollEvent event(xoffset, yoffset);
        m_eventSystem->Publish(event);
    }
}

void InputManager::HandleCharEvent(unsigned int codepoint) {
    if (m_eventSystem) {
        CharEvent event(codepoint);
        m_eventSystem->Publish(event);
    }
}

void InputManager::HandleWindowResizeEvent(int width, int height) {
    if (m_eventSystem) {
        WindowResizeEvent event(width, height);
        m_eventSystem->Publish(event);
    }
}

void InputManager::HandleWindowCloseEvent() {
    if (m_eventSystem) {
        WindowCloseEvent event;
        m_eventSystem->Publish(event);
    }
}