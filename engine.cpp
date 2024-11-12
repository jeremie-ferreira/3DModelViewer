#include "engine.h"
#include "fileUtils.h"

Engine::Engine(int screenWidth, int screenHeight) {

	// Get config values from config.ini
	// ---------------------------------
	FileUtils::ConfigMap configMap = FileUtils::readConfigFile("config.ini");
	std::string folderModels = FileUtils::getValue(configMap, "folder.models");
	std::string folderEnvironments = FileUtils::getValue(configMap, "folder.environments");
	std::string defaultModel = FileUtils::getValue(configMap, "default.model");
	std::string defaultEnvironment = FileUtils::getValue(configMap, "default.environment");

	_displayManager.init(screenWidth, screenHeight, &_eventBus, folderModels, folderEnvironments, defaultModel, defaultEnvironment);
	_inputManager.init(&_eventBus);
	_renderer.init(screenWidth, screenHeight);
	_scene.init(&_eventBus, screenWidth / (float)screenHeight);

	// Events management
	// -----------------
	// quit application
	_eventBus.subscribe(EventType::Quit, [&](const Event& event) {
		_running = false;
		});
	// move view
	_eventBus.subscribe(EventType::Move, [&](const Event& event) {
		_scene.camera.move(event.vec2);
		});
	// zoom view
	_eventBus.subscribe(EventType::Zoom, [&](const Event& event) {
		_scene.camera.zoom(event.intValue);
		});
	// resize sdl window
	_eventBus.subscribe(EventType::ResizeSdlWindow, [&](const Event& event) {
		_displayManager.resizeWindow();
		});
	// Resize camera
	_eventBus.subscribe(EventType::ResizeWindow, [&](const Event& event) {
		_renderer.resizeViewport(event.vec2);
		_scene.camera.updateRatio(event.vec2.x / (float)event.vec2.y);
		});
	// load new 3D model
	_eventBus.subscribe(EventType::LoadGlb, [&](const Event& event) {
		_scene.loadGlb(event.strValue);
		});
	// load new environment
	_eventBus.subscribe(EventType::LoadEnvironment, [&](const Event& event) {
		_renderer.loadEnvironment(event.strValue);
		});
	// change dipslay mode
	_eventBus.subscribe(EventType::ChangeDisplayMode, [&](const Event& event) {
		_renderer.setRenderMode(event.intValue);
		});
	// change background visibility
	_eventBus.subscribe(EventType::ShowBackgroundState, [&](const Event& event) {
		_renderer.setShowBackground(event.boolValue);
		});
	// change environment intensity
	_eventBus.subscribe(EventType::UpdateEnvIntensity, [&](const Event& event) {
		_renderer.setEnvIntensity(event.floatValue);
		});
	// load mesh data to GPU
	_eventBus.subscribe(EventType::LoadGpuMeshes, [&](const Event& event) {
		_renderer.loadMeshes(_scene.getMeshes());
		});
	// clear mesh data from GPU
	_eventBus.subscribe(EventType::ClearGpuMeshesAndTextures, [&](const Event& event) {
		_renderer.clearMeshes(_scene.getMeshes());
		_renderer.clearTextures(_scene.getMaterials());
		});
	// load texture data to GPU
	_eventBus.subscribe(EventType::LoadTextureRenderData, [&](const Event& event) {
		_renderer.loadTextureData(event.textureBindingEvent);
		});

	// Load first environment and model
	// --------------------------------
	_renderer.loadEnvironment(folderEnvironments + "/" + defaultEnvironment);
	_scene.loadGlb(folderModels + "/" + defaultModel);
}

void Engine::loop() {
	// main loop
	while (_running) {
		_inputManager.handleInputs();
		_renderer.render(_scene.getMeshes(), _scene.getOpaqueMeshes(), _scene.getTransparentMeshes(), _scene.camera);
		_displayManager.displayGui();
		_displayManager.swapWindows();
	}
}