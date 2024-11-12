#pragma once
#include "inputManager.h"
#include "displayManager.h"
#include "event.h"
#include "scene.h"
#include "renderer.h"

/**
 * The Engine class serves as the main controller for the application, managing
 * the main loop, rendering, scene, input, display, and event handling.
 */
class Engine {
public:
	/**
	 * Constructor that initializes the engine with screen width and height.
	 * Initialize all the components
	 * @param screenWidth width of the application window in pixels
	 * @param screenHeight height of the application window in pixels
	 */
	Engine(int screenWidth, int screenHeight);

	/**
	 * Starts the main application loop
	 */
	void loop();
private:
	// Renderer responsible for rendering the scene onto the screen
	Renderer _renderer;

	// The scene object that holds all elements in the 3D world to be rendered
	Scene _scene;

	// Manages user input (keyboard, mouse, etc.) and processes input events
	InputManager _inputManager;

	// Responsible for creating and managing the application window and GUI
	DisplayManager _displayManager;

	// Handles the event system, allowing different parts of the application to 
	// communicate through events.
	EventBus _eventBus;
	
	// main loop condition
	bool _running = true;
};

