#pragma once

#include <RmlUi/Core.h>
#include <string>
#include <memory>

class RmlUISystem;

/**
 * UIDocument wrapper class for RML document lifecycle management.
 * This class handles:
 * - RML file loading with CSS stylesheet integration
 * - Document visibility management and element access utilities
 * - Dynamic content update and element manipulation support
 */
class UIDocument {
public:
    UIDocument(RmlUISystem* uiSystem);
    ~UIDocument();

    // Document lifecycle
    bool LoadFromFile(const std::string& path);
    void Unload();
    bool IsLoaded() const { return m_document != nullptr; }

    // Visibility management
    void Show();
    void Hide();
    bool IsVisible() const { return m_visible; }

    // Element access and manipulation
    Rml::Element* GetElementById(const std::string& id);
    void SetElementText(const std::string& id, const std::string& text);
    void SetElementAttribute(const std::string& id, const std::string& attr, const std::string& value);
    bool SetElementProperty(const std::string& id, const std::string& property, const std::string& value);

    // Event handling
    void AddEventListener(const std::string& elementId, const std::string& event, 
                         std::function<void(Rml::Event&)> callback);
    void RemoveEventListener(const std::string& elementId, const std::string& event);

    // Dynamic content updates
    void UpdateElement(const std::string& id, const std::string& content);
    void SetElementClass(const std::string& id, const std::string& className, bool add = true);

    // Document properties
    const std::string& GetPath() const { return m_path; }
    Rml::ElementDocument* GetRmlDocument() const { return m_document; }

private:
    // Event listener wrapper
    class EventListenerWrapper : public Rml::EventListener {
    public:
        EventListenerWrapper(std::function<void(Rml::Event&)> callback);
        void ProcessEvent(Rml::Event& event) override;

    private:
        std::function<void(Rml::Event&)> m_callback;
    };

    RmlUISystem* m_uiSystem;
    Rml::ElementDocument* m_document = nullptr;
    std::string m_path;
    bool m_visible = false;

    // Event listeners management
    std::vector<std::unique_ptr<EventListenerWrapper>> m_eventListeners;
};