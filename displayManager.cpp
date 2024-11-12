#include "displayManager.h"
#include <iostream>
#include <glad/glad.h>
#include <imgui_impl_sdl2.h>
#include <imgui_impl_opengl3.h>
#include <filesystem>

void DisplayManager::init(int screenWidth, int screenHeight, EventBus* eventBus, std::string folderModels, std::string folderEnvironments, std::string defaultModel, std::string defaultEnv) {
    _screenWidth = screenWidth;
    _screenHeight = screenHeight;
    _eventBus = eventBus;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        std::cout << "SDL2 could not initialize video" << std::endl;
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    _sdlWindow = SDL_CreateWindow("OpenGL Window", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (_sdlWindow == nullptr) {
        std::cout << "Error creating SDL Window" << std::endl;
        exit(1);
    }

    _openGlContext = SDL_GL_CreateContext(_sdlWindow);
    if (_openGlContext == nullptr) {
        std::cout << "Error creating OpenGL Context" << std::endl;
        exit(1);
    }
    // initialize glad
    if (!gladLoadGLLoader(SDL_GL_GetProcAddress)) {
        std::cout << "Error initializing glad" << std::endl;
        exit(1);
    }

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(_sdlWindow, _openGlContext);
    ImGui_ImplOpenGL3_Init("#version 410");

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 4.f;
    style.WindowRounding = 4.f;
    _io = &ImGui::GetIO();
    _io->Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Arial.ttf", 24.f);

    // directories
    _modelPath = folderModels;
    _texturePath = folderEnvironments;
    _files = getFilesInDirectory(_modelPath, ".glb");
    _envFiles = getFilesInDirectory(_texturePath, ".hdr");
    std::vector exrFiles = getFilesInDirectory(_texturePath, ".exr");
    _envFiles.insert(_envFiles.end(), exrFiles.begin(), exrFiles.end());

    for (int n = 0; n < _files.size(); n++) {
        if (_files[n].c_str() == defaultModel) {
            _fileSelectedId = n; break;
        }
    }

    for (int n = 0; n < _envFiles.size(); n++) {
        if (_envFiles[n].c_str() == defaultEnv) {
            _envSelectedId = n; break;
        }
    }
}

void DisplayManager::cleanup() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyWindow(_sdlWindow);
    SDL_Quit();
}

void DisplayManager::resizeWindow() {
    SDL_GetWindowSize(_sdlWindow, &_screenWidth, &_screenHeight);
    printf("Resizing window to: %d x %d\n", _screenWidth, _screenHeight);
    SDL_SetWindowSize(_sdlWindow, _screenWidth, _screenHeight);
    _eventBus->publish(Event(EventType::ResizeWindow, glm::vec2(_screenWidth, _screenHeight)));
    glViewport(0, 0, _screenHeight, _screenWidth);
}

void DisplayManager::swapWindows() {
    SDL_GL_SwapWindow(_sdlWindow);
}

void DisplayManager::displayGui() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    
    // config window
    ImGui::Begin("Config");

    float itemWidth = 250;

    // render mode combo
    const char* renderModeItems[15] = { "PBR", "Albedo", "Normal", "Metallic", "Roughness", "F", "kD", "diffuse", "ambient", "irradiance", "prefilteredColor", "brdf x", "brdf y", "specular", "PBR Lights"};
    const char* renderModeComboPreviewValue = renderModeItems[_renderModeSelectedId];
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::BeginCombo("DisplayMode", renderModeComboPreviewValue, 0)) {
        for (int n = 0; n < IM_ARRAYSIZE(renderModeItems); n++) {
            const bool is_selected = (_renderModeSelectedId == n);
            if (ImGui::Selectable(renderModeItems[n], is_selected)) {
                _renderModeSelectedId = n;
                // change display mode
                _eventBus->publish(Event(EventType::ChangeDisplayMode, _renderModeSelectedId));
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // file selection combo
    const char* fileComboPreviewValue = _files[_fileSelectedId].c_str();
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::BeginCombo("3DFile", fileComboPreviewValue, 0)) {
        for (int n = 0; n < _files.size(); n++) {
            const bool is_selected = (_fileSelectedId == n);
            if (ImGui::Selectable(_files[n].c_str(), is_selected)) {
                _fileSelectedId = n;
                // load new model
                _eventBus->publish(Event(EventType::LoadGlb, (_modelPath + "/" + _files[_fileSelectedId]).c_str()));
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    // environment selection combo
    const char* envComboPreviewValue = _envFiles[_envSelectedId].c_str();
    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::BeginCombo("Environment", envComboPreviewValue, 0)) {
        for (int n = 0; n < _envFiles.size(); n++) {
            const bool is_selected = (_envSelectedId == n);
            if (ImGui::Selectable(_envFiles[n].c_str(), is_selected)) {
                _envSelectedId = n;
                // load new environment
                _eventBus->publish(Event(EventType::LoadEnvironment, (_texturePath + "/" + _envFiles[_envSelectedId]).c_str()));
            }
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndCombo();
    }

    ImGui::SetNextItemWidth(itemWidth);
    if (ImGui::SliderFloat("env intensity", &_intensity, 0.0f, 5.0f, "%.3f")) {
        _eventBus->publish(Event(EventType::UpdateEnvIntensity, _intensity));
    }

    // Check if the checkbox was toggled this frame
    if (ImGui::Checkbox("Show Background", &_showBackgroundState)) {
        std::cout << _showBackgroundState << std::endl;
        _eventBus->publish(Event(EventType::ShowBackgroundState, _showBackgroundState));
    }

    //ImGui::Text("Framerate: %g", _io->Framerate);
    ImGui::SetNextItemWidth(40);
    ImGui::InputFloat("FPS", &_io->Framerate, 0, 0, "%.0f", ImGuiInputTextFlags_ReadOnly);

    ImGui::End();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// Function to get all files in a specified directory
std::vector<std::string> DisplayManager::getFilesInDirectory(const std::string& directory, const std::string& extension) {
    std::vector<std::string> files;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) {
            files.push_back(entry.path().filename().string());
        }
    }
    return files;
}
