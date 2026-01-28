#include "EventSystem.h"
#include <iostream>

EventSystem::EventSystem() = default;

EventSystem::~EventSystem() {
    if (m_initialized) {
        Shutdown();
    }
}

bool EventSystem::Initialize() {
    if (m_initialized) {
        return true;
    }

    std::cout << "Initializing EventSystem..." << std::endl;
    
    // Clear any existing handlers and events
    m_handlers.clear();
    
    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_eventQueue.empty()) {
        m_eventQueue.pop();
    }
    
    m_initialized = true;
    std::cout << "EventSystem initialized" << std::endl;
    return true;
}

void EventSystem::Update(float deltaTime) {
    if (!m_initialized) {
        return;
    }
    
    ProcessEvents();
}

void EventSystem::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    std::cout << "Shutting down EventSystem..." << std::endl;
    
    // Clear all handlers
    m_handlers.clear();
    
    // Clear event queue
    std::lock_guard<std::mutex> lock(m_queueMutex);
    while (!m_eventQueue.empty()) {
        m_eventQueue.pop();
    }
    
    m_initialized = false;
    std::cout << "EventSystem shutdown complete" << std::endl;
}

void EventSystem::PublishEvent(std::unique_ptr<Event> event) {
    std::lock_guard<std::mutex> lock(m_queueMutex);
    m_eventQueue.push(std::move(event));
}

void EventSystem::ProcessEvents() {
    std::queue<std::unique_ptr<Event>> eventsToProcess;
    
    // Move events from the queue to local processing queue
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        eventsToProcess.swap(m_eventQueue);
    }
    
    // Process events without holding the mutex
    while (!eventsToProcess.empty()) {
        auto& event = eventsToProcess.front();
        std::string eventType = event->GetType();
        
        // Dispatch to all matching handlers
        for (const auto& handler : m_handlers) {
            if (handler.eventType == eventType) {
                try {
                    handler.handler(*event);
                } catch (const std::exception& e) {
                    std::cerr << "Error processing event " << eventType << ": " << e.what() << std::endl;
                }
            }
        }
        
        eventsToProcess.pop();
    }
}