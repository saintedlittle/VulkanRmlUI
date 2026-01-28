#pragma once

#include "../Engine/Engine.h"
#include <functional>
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <string>
#include <typeinfo>
#include <unordered_map>

class Event {
public:
    virtual ~Event() = default;
    virtual std::string GetType() const = 0;
};

class EventSystem : public IEngineModule {
public:
    EventSystem();
    ~EventSystem();

    // IEngineModule interface
    bool Initialize() override;
    void Update(float deltaTime) override;
    void Shutdown() override;
    const char* GetName() const override { return "EventSystem"; }
    int GetInitializationOrder() const override { return 100; }

    // Event handling
    template<typename T>
    void Subscribe(std::function<void(const T&)> handler);
    
    template<typename T>
    void Publish(const T& event);
    
    void PublishEvent(std::unique_ptr<Event> event);
    
    void ProcessEvents();

private:
    struct EventHandler {
        std::function<void(const Event&)> handler;
        std::string eventType;
    };
    
    std::vector<EventHandler> m_handlers;
    std::queue<std::unique_ptr<Event>> m_eventQueue;
    std::mutex m_queueMutex;
    bool m_initialized = false;
};

// Template implementations
template<typename T>
void EventSystem::Subscribe(std::function<void(const T&)> handler) {
    EventHandler eventHandler;
    eventHandler.eventType = typeid(T).name();
    eventHandler.handler = [handler](const Event& event) {
        const T* typedEvent = dynamic_cast<const T*>(&event);
        if (typedEvent) {
            handler(*typedEvent);
        }
    };
    
    m_handlers.push_back(eventHandler);
}

template<typename T>
void EventSystem::Publish(const T& event) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_eventQueue.push(std::make_unique<T>(event));
}