#include "RmlUISystem.h"
#include "VulkanRmlRenderer.h"
#include "UIDocument.h"
#include "../Vulkan/VulkanRenderer.h"
#include "../Vulkan/ResourceManager.h"
#include "../Assets/AssetManager.h"
#include "../Core/EventSystem.h"
#include "../Core/InputEvents.h"
#include <RmlUi/Core.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <fstream>

// RmlUI System Interface implementation
class RmlUISystem::SystemInterface : public Rml::SystemInterface {
public:
    double GetElapsedTime() override {
        return glfwGetTime();
    }
    
    void SetMouseCursor(const Rml::String& cursor_name) override {
        // TODO: Implement cursor changes if needed
    }
    
    void SetClipboardText(const Rml::String& text) override {
        glfwSetClipboardString(nullptr, text.c_str());
    }
    
    void GetClipboardText(Rml::String& text) override {
        const char* clipboard_text = glfwGetClipboardString(nullptr);
        if (clipboard_text) {
            text = clipboard_text;
        }
    }
    
    bool LogMessage(Rml::Log::Type type, const Rml::String& message) override {
        const char* type_str = "";
        switch (type) {
            case Rml::Log::LT_ALWAYS:   type_str = "ALWAYS"; break;
            case Rml::Log::LT_ERROR:    type_str = "ERROR"; break;
            case Rml::Log::LT_ASSERT:   type_str = "ASSERT"; break;
            case Rml::Log::LT_WARNING:  type_str = "WARNING"; break;
            case Rml::Log::LT_INFO:     type_str = "INFO"; break;
            case Rml::Log::LT_DEBUG:    type_str = "DEBUG"; break;
            case Rml::Log::LT_MAX:      type_str = "MAX"; break;
        }
        
        std::cout << "[RmlUI " << type_str << "] " << message << std::endl;
        return true;
    }
};

// RmlUI File Interface implementation
class RmlUISystem::FileInterface : public Rml::FileInterface {
public:
    Rml::FileHandle Open(const Rml::String& path) override {
        std::ifstream* file = new std::ifstream(path, std::ios::binary);
        if (!file->is_open()) {
            delete file;
            return 0;
        }
        return reinterpret_cast<Rml::FileHandle>(file);
    }
    
    void Close(Rml::FileHandle file) override {
        if (file) {
            delete reinterpret_cast<std::ifstream*>(file);
        }
    }
    
    size_t Read(void* buffer, size_t size, Rml::FileHandle file) override {
        if (!file) return 0;
        
        std::ifstream* stream = reinterpret_cast<std::ifstream*>(file);
        stream->read(static_cast<char*>(buffer), size);
        return static_cast<size_t>(stream->gcount());
    }
    
    bool Seek(Rml::FileHandle file, long offset, int origin) override {
        if (!file) return false;
        
        std::ifstream* stream = reinterpret_cast<std::ifstream*>(file);
        
        std::ios_base::seekdir dir;
        switch (origin) {
            case SEEK_SET: dir = std::ios_base::beg; break;
            case SEEK_CUR: dir = std::ios_base::cur; break;
            case SEEK_END: dir = std::ios_base::end; break;
            default: return false;
        }
        
        stream->seekg(offset, dir);
        return !stream->fail();
    }
    
    size_t Tell(Rml::FileHandle file) override {
        if (!file) return 0;
        
        std::ifstream* stream = reinterpret_cast<std::ifstream*>(file);
        return static_cast<size_t>(stream->tellg());
    }
};

RmlUISystem::RmlUISystem(VulkanRenderer* renderer, AssetManager* assetManager, ResourceManager* resourceManager)
    : m_renderer(renderer), m_assetManager(assetManager), m_resourceManager(resourceManager) {
}

RmlUISystem::~RmlUISystem() {
    if (m_initialized) {
        Shutdown();
    }
}

bool RmlUISystem::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing RmlUISystem..." << std::endl;
    
    try {
        // Initialize RmlUI
        if (!InitializeRmlUI()) {
            std::cerr << "Failed to initialize RmlUI" << std::endl;
            return false;
        }
        
        // Create Vulkan renderer
        m_rmlRenderer = std::make_unique<VulkanRmlRenderer>(m_renderer, m_resourceManager);
        if (!m_rmlRenderer->Initialize()) {
            std::cerr << "Failed to initialize VulkanRmlRenderer" << std::endl;
            return false;
        }
        
        // Set render interface
        Rml::SetRenderInterface(m_rmlRenderer.get());
        
        // Create context
        if (!CreateContext()) {
            std::cerr << "Failed to create RmlUI context" << std::endl;
            return false;
        }
        
        // Load default font using AssetManager
        if (!m_assetManager->LoadFont("fonts/Roboto-Regular.ttf", "Roboto")) {
            std::cerr << "Failed to load default font" << std::endl;
            return false;
        }
        
        m_initialized = true;
        std::cout << "RmlUISystem initialized successfully" << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "RmlUISystem initialization failed: " << e.what() << std::endl;
        Shutdown();
        return false;
    }
}

void RmlUISystem::Update(float deltaTime) {
    if (!m_initialized || !m_context) {
        return;
    }
    
    // Update RmlUI context
    m_context->Update();
}

void RmlUISystem::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down RmlUISystem..." << std::endl;
    
    // Unload all documents
    for (auto& [path, document] : m_loadedDocuments) {
        if (document) {
            document->Close();
        }
    }
    m_loadedDocuments.clear();
    
    // Cleanup context
    if (m_context) {
        Rml::RemoveContext(m_context->GetName());
        m_context = nullptr;
    }
    
    // Cleanup renderer
    if (m_rmlRenderer) {
        m_rmlRenderer->Cleanup();
        m_rmlRenderer.reset();
    }
    
    // Shutdown RmlUI
    Rml::Shutdown();
    
    m_initialized = false;
    std::cout << "RmlUISystem shutdown complete" << std::endl;
}

bool RmlUISystem::LoadFont(const std::string& fontPath, const std::string& fontName) {
    if (!m_initialized || !m_assetManager) {
        return false;
    }
    
    return m_assetManager->LoadFont(fontPath, fontName);
}

bool RmlUISystem::LoadStylesheet(const std::string& stylesheetPath) {
    if (!m_initialized || !m_context) {
        return false;
    }
    
    auto stylesheet = Rml::Factory::InstanceStyleSheetFile(stylesheetPath);
    if (stylesheet) {
        // Apply stylesheet to context (this is a simplified approach)
        // In a real implementation, you might want to manage stylesheets more carefully
        return true;
    }
    return false;
}

Rml::ElementDocument* RmlUISystem::LoadDocument(const std::string& rmlPath) {
    if (!m_initialized || !m_context) {
        return nullptr;
    }
    
    // Check if already loaded
    auto it = m_loadedDocuments.find(rmlPath);
    if (it != m_loadedDocuments.end()) {
        return it->second;
    }
    
    // Load document
    Rml::ElementDocument* document = m_context->LoadDocument(rmlPath);
    if (document) {
        m_loadedDocuments[rmlPath] = document;
    }
    
    return document;
}

void RmlUISystem::UnloadDocument(Rml::ElementDocument* document) {
    if (!document) {
        return;
    }
    
    // Find and remove from loaded documents
    for (auto it = m_loadedDocuments.begin(); it != m_loadedDocuments.end(); ++it) {
        if (it->second == document) {
            document->Close();
            m_loadedDocuments.erase(it);
            break;
        }
    }
}

void RmlUISystem::ShowDocument(Rml::ElementDocument* document) {
    if (document) {
        document->Show();
    }
}

void RmlUISystem::HideDocument(Rml::ElementDocument* document) {
    if (document) {
        document->Hide();
    }
}

void RmlUISystem::ProcessKeyEvent(int key, int action, int mods) {
    if (!m_initialized || !m_context) {
        return;
    }
    
    Rml::Input::KeyIdentifier rmlKey = ConvertKey(key);
    int rmlMods = ConvertKeyModifiers(mods);
    
    if (action == GLFW_PRESS) {
        m_context->ProcessKeyDown(rmlKey, rmlMods);
    } else if (action == GLFW_RELEASE) {
        m_context->ProcessKeyUp(rmlKey, rmlMods);
    }
}

void RmlUISystem::ProcessMouseButtonEvent(int button, int action, int mods) {
    if (!m_initialized || !m_context) {
        return;
    }
    
    int rmlButton = button; // GLFW and RmlUI use same button indices
    int rmlMods = ConvertKeyModifiers(mods);
    
    if (action == GLFW_PRESS) {
        m_context->ProcessMouseButtonDown(rmlButton, rmlMods);
    } else if (action == GLFW_RELEASE) {
        m_context->ProcessMouseButtonUp(rmlButton, rmlMods);
    }
}

void RmlUISystem::ProcessMouseMoveEvent(double xpos, double ypos) {
    if (!m_initialized || !m_context) {
        return;
    }
    
    m_mouseX = xpos;
    m_mouseY = ypos;
    
    m_context->ProcessMouseMove(static_cast<int>(xpos), static_cast<int>(ypos), 0);
}

void RmlUISystem::ProcessScrollEvent(double xoffset, double yoffset) {
    if (!m_initialized || !m_context) {
        return;
    }
    
    m_context->ProcessMouseWheel(static_cast<float>(-yoffset), 0);
}

void RmlUISystem::ProcessCharEvent(unsigned int codepoint) {
    if (!m_initialized || !m_context) {
        return;
    }
    
    m_context->ProcessTextInput(codepoint);
}

void RmlUISystem::Render(VkCommandBuffer commandBuffer, VkRenderPass renderPass,
                        uint32_t framebufferWidth, uint32_t framebufferHeight) {
    if (!m_initialized || !m_context || !m_rmlRenderer) {
        return;
    }
    
    // Begin frame for renderer
    m_rmlRenderer->BeginFrame(commandBuffer, renderPass, framebufferWidth, framebufferHeight);
    
    // Render context
    m_context->Render();
    
    // End frame
    m_rmlRenderer->EndFrame();
}

// Private helper methods
bool RmlUISystem::InitializeRmlUI() {
    // Create system interface
    m_systemInterface = std::make_unique<SystemInterface>();
    Rml::SetSystemInterface(m_systemInterface.get());
    
    // Create file interface
    m_fileInterface = std::make_unique<FileInterface>();
    Rml::SetFileInterface(m_fileInterface.get());
    
    // Initialize RmlUI
    if (!Rml::Initialise()) {
        std::cerr << "Failed to initialize RmlUI" << std::endl;
        return false;
    }
    
    return true;
}

bool RmlUISystem::CreateContext() {
    // Create context with window dimensions
    // TODO: Get actual window dimensions from renderer
    int width = 1920;
    int height = 1080;
    
    m_context = Rml::CreateContext("main", Rml::Vector2i(width, height));
    if (!m_context) {
        std::cerr << "Failed to create RmlUI context" << std::endl;
        return false;
    }
    
    return true;
}

void RmlUISystem::SetupEventHandlers() {
    // TODO: Set up event handlers for UI interactions
    // This will be implemented when we add navigation support
}

Rml::Input::KeyIdentifier RmlUISystem::ConvertKey(int glfwKey) {
    // Convert GLFW key codes to RmlUI key identifiers
    switch (glfwKey) {
        case GLFW_KEY_SPACE: return Rml::Input::KI_SPACE;
        case GLFW_KEY_0: return Rml::Input::KI_0;
        case GLFW_KEY_1: return Rml::Input::KI_1;
        case GLFW_KEY_2: return Rml::Input::KI_2;
        case GLFW_KEY_3: return Rml::Input::KI_3;
        case GLFW_KEY_4: return Rml::Input::KI_4;
        case GLFW_KEY_5: return Rml::Input::KI_5;
        case GLFW_KEY_6: return Rml::Input::KI_6;
        case GLFW_KEY_7: return Rml::Input::KI_7;
        case GLFW_KEY_8: return Rml::Input::KI_8;
        case GLFW_KEY_9: return Rml::Input::KI_9;
        case GLFW_KEY_A: return Rml::Input::KI_A;
        case GLFW_KEY_B: return Rml::Input::KI_B;
        case GLFW_KEY_C: return Rml::Input::KI_C;
        case GLFW_KEY_D: return Rml::Input::KI_D;
        case GLFW_KEY_E: return Rml::Input::KI_E;
        case GLFW_KEY_F: return Rml::Input::KI_F;
        case GLFW_KEY_G: return Rml::Input::KI_G;
        case GLFW_KEY_H: return Rml::Input::KI_H;
        case GLFW_KEY_I: return Rml::Input::KI_I;
        case GLFW_KEY_J: return Rml::Input::KI_J;
        case GLFW_KEY_K: return Rml::Input::KI_K;
        case GLFW_KEY_L: return Rml::Input::KI_L;
        case GLFW_KEY_M: return Rml::Input::KI_M;
        case GLFW_KEY_N: return Rml::Input::KI_N;
        case GLFW_KEY_O: return Rml::Input::KI_O;
        case GLFW_KEY_P: return Rml::Input::KI_P;
        case GLFW_KEY_Q: return Rml::Input::KI_Q;
        case GLFW_KEY_R: return Rml::Input::KI_R;
        case GLFW_KEY_S: return Rml::Input::KI_S;
        case GLFW_KEY_T: return Rml::Input::KI_T;
        case GLFW_KEY_U: return Rml::Input::KI_U;
        case GLFW_KEY_V: return Rml::Input::KI_V;
        case GLFW_KEY_W: return Rml::Input::KI_W;
        case GLFW_KEY_X: return Rml::Input::KI_X;
        case GLFW_KEY_Y: return Rml::Input::KI_Y;
        case GLFW_KEY_Z: return Rml::Input::KI_Z;
        case GLFW_KEY_ENTER: return Rml::Input::KI_RETURN;
        case GLFW_KEY_ESCAPE: return Rml::Input::KI_ESCAPE;
        case GLFW_KEY_BACKSPACE: return Rml::Input::KI_BACK;
        case GLFW_KEY_TAB: return Rml::Input::KI_TAB;
        case GLFW_KEY_LEFT_SHIFT: return Rml::Input::KI_LSHIFT;
        case GLFW_KEY_RIGHT_SHIFT: return Rml::Input::KI_RSHIFT;
        case GLFW_KEY_LEFT_CONTROL: return Rml::Input::KI_LCONTROL;
        case GLFW_KEY_RIGHT_CONTROL: return Rml::Input::KI_RCONTROL;
        case GLFW_KEY_LEFT_ALT: return Rml::Input::KI_LMETA;
        case GLFW_KEY_RIGHT_ALT: return Rml::Input::KI_RMETA;
        case GLFW_KEY_LEFT: return Rml::Input::KI_LEFT;
        case GLFW_KEY_RIGHT: return Rml::Input::KI_RIGHT;
        case GLFW_KEY_UP: return Rml::Input::KI_UP;
        case GLFW_KEY_DOWN: return Rml::Input::KI_DOWN;
        default: return Rml::Input::KI_UNKNOWN;
    }
}

int RmlUISystem::ConvertKeyModifiers(int glfwMods) {
    int rmlMods = 0;
    
    if (glfwMods & GLFW_MOD_SHIFT) {
        rmlMods |= Rml::Input::KM_SHIFT;
    }
    if (glfwMods & GLFW_MOD_CONTROL) {
        rmlMods |= Rml::Input::KM_CTRL;
    }
    if (glfwMods & GLFW_MOD_ALT) {
        rmlMods |= Rml::Input::KM_ALT;
    }
    if (glfwMods & GLFW_MOD_SUPER) {
        rmlMods |= Rml::Input::KM_META;
    }
    
    return rmlMods;
}
std::unique_ptr<UIDocument> RmlUISystem::CreateUIDocument() {
    if (!m_initialized) {
        std::cerr << "RmlUISystem: Cannot create UIDocument - system not initialized" << std::endl;
        return nullptr;
    }
    
    return std::make_unique<UIDocument>(this);
}