#pragma once
#include "event.h"
#include <imgui.h>
#include <SDL.h>

/**
 * The DisplayManager class manages window creation, OpenGL context, and GUI rendering.
 * It handles the display settings, file directories, and user interface.
 */
class DisplayManager {
public:
    /**
     * Default constructor for DisplayManager.
     */
    DisplayManager() {}

    /**
     * Initializes the DisplayManager with window settings, directories, and default model/environment.
     * @param screenWidth Width of the display window.
     * @param screenHeight Height of the display window.
     * @param eventBus Pointer to the EventBus for event handling.
     * @param folderModels Directory path for model files.
     * @param folderEnvironments Directory path for environment files.
     * @param defaultModel Path to the default model file.
     * @param defaultEnv Path to the default environment file.
     */
    void init(int screenWidth, int screenHeight, EventBus* eventBus, std::string folderModels, std::string folderEnvironments, std::string defaultModel, std::string defaultEnv);

    /**
     * Cleans up resources, including the SDL window and OpenGL context.
     */
    void cleanup();

    /**
     * Resizes the window based on new dimensions.
     */
    void resizeWindow();

    /**
     * Swaps the OpenGL window buffers to display the rendered content.
     */
    void swapWindows();

    /**
     * Renders the GUI elements on the screen.
     */
    void displayGui();

private:
    SDL_Window* _sdlWindow;            // Pointer to the SDL window
    SDL_GLContext _openGlContext;      // OpenGL context associated with the SDL window
    int _screenHeight, _screenWidth;   // Screen dimensions
    EventBus* _eventBus;               // Pointer to the EventBus for handling events
    ImGuiIO* _io;                      // Pointer to ImGui's IO handler for UI interactions

    bool _showBackgroundState = true;  // background checkbox state
    int _renderModeSelectedId = 0;     // ID for the selected render mode in the UI
    int _fileSelectedId = 0;           // ID for the selected file in the model directory
    int _envSelectedId = 0;            // ID for the selected environment in the environment directory
    float _intensity = 1.0f;           // environment map intensity value

    std::string _modelPath;            // Path to the selected model file
    std::string _texturePath;          // Path to the selected texture file
    std::vector<std::string> _files;   // List of available model files
    std::vector<std::string> _envFiles; // List of available environment files

    /**
     * Retrieves files from a specified directory with a given file extension.
     * @param directory The directory to search in.
     * @param extension The file extension to filter files.
     * @return A vector of file paths that match the specified extension.
     */
    std::vector<std::string> getFilesInDirectory(const std::string& directory, const std::string& extension);
};
