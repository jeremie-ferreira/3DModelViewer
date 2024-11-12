# 3D Model Viewer

A C++ 3D Model Viewer with Physically-Based Rendering (PBR), supporting model loading, environment-based lighting, and user-friendly camera controls. This application provides an interactive way to view PBR-compliant 3D models with a range of material and rendering options.

## Features

- **Model Loading**: Open and display 3D models that are compliant with Physically-Based Rendering (PBR).
- **Environment Selection**: Choose from a set of environment textures for Image-Based Lighting (IBL) and background.
- **Camera Controls**: Orbit and zoom around the model with intuitive left-drag and mouse wheel controls.
- **PBR Rendering**:
  - Includes lighting and IBL.
  - Supports transparent materials
  - Supports solid colors: diffuse, roughnessFactor, metallicFactor
  - Supports texture maps: diffuse, normal, roughness, metallic
- **Rendering Debugging**: Display various stages of the PBR rendering pipeline for in-depth analysis.

## Getting Started

### Prerequisites

The following libraries are required to build and run the 3D Model Viewer:

- **SDL2**: Window and input handling.
- **GLAD**: OpenGL loader.
- **ImGui**: User interface for model and environment selection.
- **GLM**: Mathematics library for 3D transformations.
- **Assimp**: 3D model import library.
- **STB**: Image loading (via `stb_image.h`).
- **OpenEXR**: HDR texture support for environment maps.

Ensure these dependencies are installed and linked appropriately in your development environment.

### Configuration

To specify the default folders and models, create a `config.ini` file in the root of the project with the following options:

```ini
folder.models="path/to/your/models"
folder.environments="path/to/your/environments"
default.model="model_name"
default.environment="environment_name"
```

### Building the Project

1. Clone the repository:

   ```bash
   git clone https://github.com/yourusername/3d-model-viewer.git
   cd 3d-model-viewer
   ```

2. Build the project using your preferred build system (e.g., CMake).

3. Run the executable to start the 3D Model Viewer.

### Running the Project

Once the project is built, launch the executable to open the model viewer. Use the UI to select models and environments, orbit around the model by left-dragging, and zoom using the mouse wheel.

## Credits

This project’s PBR and IBL rendering techniques are based on the tutorials from [LearnOpenGL](https://learnopengl.com/PBR/IBL/Specular-IBL), which provides an excellent foundation for implementing realistic lighting and reflections in OpenGL.
