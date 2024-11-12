#pragma once
#include "event.h"
#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <imgui.h>

/**
 * The InputManager class handles user input events, including mouse and keyboard interactions.
 * It processes input and dispatches events through the event bus.
 */
class InputManager {
public:
    /**
     * Initializes the InputManager with an event bus for dispatching input events.
     * @param eventBus Pointer to the EventBus used to dispatch events.
     */
    void init(EventBus* eventBus);

    /**
     * Processes and handles user inputs, including mouse and keyboard events.
     */
    void handleInputs();

private:
    bool _isDragging = false;        // Tracks whether a dragging action is in progress
    glm::vec2 _lastMousePos;         // Stores the last recorded mouse position
    EventBus* _eventBus;             // Pointer to the EventBus for event dispatching
};
