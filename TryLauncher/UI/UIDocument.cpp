#include "UIDocument.h"
#include "RmlUISystem.h"
#include <iostream>

UIDocument::EventListenerWrapper::EventListenerWrapper(std::function<void(Rml::Event&)> callback)
    : m_callback(std::move(callback)) {
}

void UIDocument::EventListenerWrapper::ProcessEvent(Rml::Event& event) {
    if (m_callback) {
        m_callback(event);
    }
}

UIDocument::UIDocument(RmlUISystem* uiSystem)
    : m_uiSystem(uiSystem) {
}

UIDocument::~UIDocument() {
    Unload();
}

bool UIDocument::LoadFromFile(const std::string& path) {
    if (m_document) {
        Unload();
    }

    if (!m_uiSystem) {
        std::cerr << "UIDocument: No UI system available" << std::endl;
        return false;
    }

    m_document = m_uiSystem->LoadDocument(path);
    if (!m_document) {
        std::cerr << "UIDocument: Failed to load document: " << path << std::endl;
        return false;
    }

    m_path = path;
    std::cout << "UIDocument: Loaded document: " << path << std::endl;
    return true;
}

void UIDocument::Unload() {
    if (m_document && m_uiSystem) {
        // Remove all event listeners
        m_eventListeners.clear();
        
        // Hide document if visible
        if (m_visible) {
            Hide();
        }
        
        // Unload document
        m_uiSystem->UnloadDocument(m_document);
        m_document = nullptr;
        m_path.clear();
    }
}

void UIDocument::Show() {
    if (m_document && !m_visible) {
        m_uiSystem->ShowDocument(m_document);
        m_visible = true;
        std::cout << "UIDocument: Showing document: " << m_path << std::endl;
    }
}

void UIDocument::Hide() {
    if (m_document && m_visible) {
        m_uiSystem->HideDocument(m_document);
        m_visible = false;
        std::cout << "UIDocument: Hiding document: " << m_path << std::endl;
    }
}

Rml::Element* UIDocument::GetElementById(const std::string& id) {
    if (!m_document) {
        return nullptr;
    }

    return m_document->GetElementById(id);
}

void UIDocument::SetElementText(const std::string& id, const std::string& text) {
    Rml::Element* element = GetElementById(id);
    if (element) {
        element->SetInnerRML(text);
    } else {
        std::cerr << "UIDocument: Element not found: " << id << std::endl;
    }
}

void UIDocument::SetElementAttribute(const std::string& id, const std::string& attr, const std::string& value) {
    Rml::Element* element = GetElementById(id);
    if (element) {
        element->SetAttribute(attr, value);
    } else {
        std::cerr << "UIDocument: Element not found: " << id << std::endl;
    }
}

bool UIDocument::SetElementProperty(const std::string& id, const std::string& property, const std::string& value) {
    Rml::Element* element = GetElementById(id);
    if (element) {
        return element->SetProperty(property, value);
    } else {
        std::cerr << "UIDocument: Element not found: " << id << std::endl;
        return false;
    }
}

void UIDocument::AddEventListener(const std::string& elementId, const std::string& event,
                                 std::function<void(Rml::Event&)> callback) {
    Rml::Element* element = GetElementById(elementId);
    if (!element) {
        std::cerr << "UIDocument: Cannot add event listener - element not found: " << elementId << std::endl;
        return;
    }

    auto listener = std::make_unique<EventListenerWrapper>(std::move(callback));
    element->AddEventListener(event, listener.get());
    m_eventListeners.push_back(std::move(listener));

    std::cout << "UIDocument: Added event listener for " << elementId << ":" << event << std::endl;
}

void UIDocument::RemoveEventListener(const std::string& elementId, const std::string& event) {
    Rml::Element* element = GetElementById(elementId);
    if (element) {
        // Note: RmlUI doesn't provide a direct way to remove specific listeners
        // This is a limitation of the current implementation
        std::cout << "UIDocument: Event listener removal not fully implemented for " << elementId << ":" << event << std::endl;
    }
}

void UIDocument::UpdateElement(const std::string& id, const std::string& content) {
    Rml::Element* element = GetElementById(id);
    if (element) {
        element->SetInnerRML(content);
        std::cout << "UIDocument: Updated element " << id << " with new content" << std::endl;
    } else {
        std::cerr << "UIDocument: Element not found for update: " << id << std::endl;
    }
}

void UIDocument::SetElementClass(const std::string& id, const std::string& className, bool add) {
    Rml::Element* element = GetElementById(id);
    if (element) {
        if (add) {
            element->SetClass(className, true);
        } else {
            element->SetClass(className, false);
        }
        std::cout << "UIDocument: " << (add ? "Added" : "Removed") << " class " << className 
                  << " to/from element " << id << std::endl;
    } else {
        std::cerr << "UIDocument: Element not found for class modification: " << id << std::endl;
    }
}