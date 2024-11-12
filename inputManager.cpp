#include "inputManager.h"
#include "event.h"
#include <SDL.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <filesystem>

void InputManager::init(EventBus* eventBus) {
    _eventBus = eventBus;
}

void InputManager::handleInputs() {
    // inputs
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL2_ProcessEvent(&event);
        if (event.type == SDL_QUIT) {
            std::cout << "Goodbye!" << std::endl;
            _eventBus->publish(Event(EventType::Quit));
        }
        if (event.type == SDL_MOUSEMOTION && _isDragging) {
            glm::vec2 delta = glm::vec2(event.motion.x, event.motion.y) - _lastMousePos;
            _eventBus->publish(Event(EventType::Move, delta));
            _lastMousePos = glm::vec2(event.motion.x, event.motion.y);
        }
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT) {
            _isDragging = true;
            _lastMousePos = glm::vec2(event.motion.x, event.motion.y);
        }
        else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT) {
            _isDragging = false;
        }
        if (event.type == SDL_MOUSEWHEEL) {
            _eventBus->publish(Event(EventType::Zoom, event.wheel.y));
        }
        if (event.type == SDL_WINDOWEVENT) {
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                _eventBus->publish(Event(EventType::ResizeSdlWindow));
            }
        }
    }
}

