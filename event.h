#pragma once
#include "mesh.h"
#include <glm/glm.hpp>
#include <SDL.h>
#include <unordered_map>
#include <vector>
#include <functional>
#include <algorithm>

/**
 * Enum class representing different types of events that can be triggered
 * within the application.
 */
enum class EventType {
    Quit,                    // Event triggered when the application should quit
    Move,                    // Event for moving objects or the camera
    ChangeDisplayMode,       // Event for changing display mode settings
    ShowBackgroundState,     // Event for changing display mode settings
    Zoom,                    // Event for zooming in or out
    ResizeSdlWindow,         // Event for resizing the SDL window
    ResizeWindow,            // Event for resizing the application's main window
    LoadGlb,                 // Event for loading a GLB model
    LoadGpuMeshes,           // Event for loading GPU mesh data
    LoadTextureRenderData,   // Event for loading texture data for rendering
    ClearGpuMeshesAndTextures, // Event for clearing GPU resources
    LoadEnvironment,         // Event for loading an environment texture
    UpdateEnvIntensity       // Update Environment intensity
};

/**
 * Structure representing the necessary information for binding a texture
 * to a material during rendering.
 */
struct TextureBindingEvent {
    TextureBindingEvent() {}

    /**
     * Initializes a TextureBindingEvent with material and texture data.
     * @param material Pointer to the material to which the texture is bound.
     * @param type The type of the texture (e.g., diffuse, normal, etc.).
     * @param imageData Pointer to the texture data in memory.
     * @param channels Number of color channels in the texture.
     * @param width Width of the texture in pixels.
     * @param height Height of the texture in pixels.
     */
    TextureBindingEvent(Material* material, TextureType type, unsigned char* imageData, int channels, int width, int height)
        : material(material), type(type), imageData(imageData), channels(channels), width(width), height(height) {}

    const unsigned char* imageData; // Pointer to the image data in memory
    int width, height, channels;    // Width, height, and color channels of the image
    Material* material;             // Pointer to the material associated with this texture
    TextureType type;               // Type of texture (Diffuse, Normal, etc.)
};

/**
 * Structure representing an event with various possible types and data payloads,
 * allowing it to hold data relevant to a wide range of event types.
 */
struct Event {
    EventType type;            // Type of event
    glm::vec2 vec2;            // 2D vector, e.g., for movement or scaling
    SDL_EventType sdlEvent;    // SDL-specific event type

    bool boolValue;            // Boolean value payload, e.g. for show background
    int intValue = 0;          // Integer value payload, e.g., for zoom level
    float floatValue = 1.0f;   // Float value payload, e.g., for scaling
    const char* strValue;      // String payload, e.g., for filenames
    TextureBindingEvent textureBindingEvent; // Payload for texture binding events

    /**
     * Constructor for generic Move events.
     * @param type The type of event.
     */
    Event(EventType type) : type(type) {}

    /**
     * Constructor for events with a 2D vector payload.
     * @param type The type of event.
     * @param vec2 The 2D vector data associated with this event.
     */
    Event(EventType type, glm::vec2 vec2)
        : type(type), vec2(vec2) {}

    /**
     * Constructor for events with a boolean payload.
     * @param type The type of event.
     * @param boolValue The boolean data associated with this event.
     */
    Event(EventType type, bool boolValue)
        : type(type), boolValue(boolValue) {}

    /**
     * Constructor for events with an integer payload.
     * @param type The type of event.
     * @param intValue The integer data associated with this event.
     */
    Event(EventType type, int intValue)
        : type(type), intValue(intValue) {}

    /**
     * Constructor for events with a float payload.
     * @param type The type of event.
     * @param floatValue The float data associated with this event.
     */
    Event(EventType type, float floatValue)
        : type(type), floatValue(floatValue) {}

    /**
     * Constructor for events with a string payload.
     * @param type The type of event.
     * @param strValue The string data associated with this event.
     */
    Event(EventType type, const char* strValue)
        : type(type), strValue(strValue) {}

    /**
     * Constructor for events with a TextureBindingEvent payload.
     * @param type The type of event.
     * @param textureBindingEvent The TextureBindingEvent data associated with this event.
     */
    Event(EventType type, TextureBindingEvent textureBindingEvent)
        : type(type), textureBindingEvent(textureBindingEvent) {}
};

/**
 * The EventBus class manages the subscription and publishing of events.
 * Subscribers register callbacks for specific event types and receive events when published.
 */
class EventBus {
public:
    using EventCallback = std::function<void(const Event&)>;

    /**
     * Subscribes a callback function to a specific event type.
     * @param type The type of event to subscribe to.
     * @param callback The callback function to invoke when an event of this type is published.
     */
    void subscribe(EventType type, EventCallback callback) {
        subscribers[type].push_back(callback);
    }

    /**
     * Publishes an event to all subscribers of the given event type.
     * @param event The event to publish to subscribers.
     */
    void publish(const Event& event) const {
        auto it = subscribers.find(event.type);
        if (it != subscribers.end()) {
            for (const auto& callback : it->second) {
                callback(event);
            }
        }
    }

private:
    std::unordered_map<EventType, std::vector<EventCallback>> subscribers; // Map of subscribers for each event type
};
