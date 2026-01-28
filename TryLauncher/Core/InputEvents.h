#pragma once

#include "EventSystem.h"
#include <string>

/**
 * Input events for GLFW integration with the event system.
 * These events are published by the input system and can be subscribed to by any module.
 */

class KeyEvent : public Event {
public:
    KeyEvent(int key, int scancode, int action, int mods)
        : m_key(key), m_scancode(scancode), m_action(action), m_mods(mods) {}
    
    std::string GetType() const override { return "KeyEvent"; }
    
    int GetKey() const { return m_key; }
    int GetScancode() const { return m_scancode; }
    int GetAction() const { return m_action; }
    int GetMods() const { return m_mods; }
    
    bool IsPressed() const { return m_action == 1; } // GLFW_PRESS
    bool IsReleased() const { return m_action == 0; } // GLFW_RELEASE
    bool IsRepeated() const { return m_action == 2; } // GLFW_REPEAT

private:
    int m_key;
    int m_scancode;
    int m_action;
    int m_mods;
};

class MouseButtonEvent : public Event {
public:
    MouseButtonEvent(int button, int action, int mods, double xpos, double ypos)
        : m_button(button), m_action(action), m_mods(mods), m_xpos(xpos), m_ypos(ypos) {}
    
    std::string GetType() const override { return "MouseButtonEvent"; }
    
    int GetButton() const { return m_button; }
    int GetAction() const { return m_action; }
    int GetMods() const { return m_mods; }
    double GetX() const { return m_xpos; }
    double GetY() const { return m_ypos; }
    
    bool IsPressed() const { return m_action == 1; } // GLFW_PRESS
    bool IsReleased() const { return m_action == 0; } // GLFW_RELEASE

private:
    int m_button;
    int m_action;
    int m_mods;
    double m_xpos;
    double m_ypos;
};

class MouseMoveEvent : public Event {
public:
    MouseMoveEvent(double xpos, double ypos, double deltaX, double deltaY)
        : m_xpos(xpos), m_ypos(ypos), m_deltaX(deltaX), m_deltaY(deltaY) {}
    
    std::string GetType() const override { return "MouseMoveEvent"; }
    
    double GetX() const { return m_xpos; }
    double GetY() const { return m_ypos; }
    double GetDeltaX() const { return m_deltaX; }
    double GetDeltaY() const { return m_deltaY; }

private:
    double m_xpos;
    double m_ypos;
    double m_deltaX;
    double m_deltaY;
};

class MouseScrollEvent : public Event {
public:
    MouseScrollEvent(double xoffset, double yoffset)
        : m_xoffset(xoffset), m_yoffset(yoffset) {}
    
    std::string GetType() const override { return "MouseScrollEvent"; }
    
    double GetXOffset() const { return m_xoffset; }
    double GetYOffset() const { return m_yoffset; }

private:
    double m_xoffset;
    double m_yoffset;
};

class CharEvent : public Event {
public:
    CharEvent(unsigned int codepoint)
        : m_codepoint(codepoint) {}
    
    std::string GetType() const override { return "CharEvent"; }
    
    unsigned int GetCodepoint() const { return m_codepoint; }

private:
    unsigned int m_codepoint;
};

class WindowResizeEvent : public Event {
public:
    WindowResizeEvent(int width, int height)
        : m_width(width), m_height(height) {}
    
    std::string GetType() const override { return "WindowResizeEvent"; }
    
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }

private:
    int m_width;
    int m_height;
};

class WindowCloseEvent : public Event {
public:
    WindowCloseEvent() = default;
    
    std::string GetType() const override { return "WindowCloseEvent"; }
};