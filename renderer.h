#pragma once
#include "event.h"
#include "mesh.h"
#include "shader.h"
#include "camera.h"
#include <vector>
#include <ImfRgba.h>

// Struct to hold environment maps for IBL (Image-Based Lighting)
struct Environment {
	GLuint prefilterMap = 0;        // Prefiltered environment map for reflections
	GLuint brdfLutTexture = 0;      // BRDF LUT texture for specular reflections
	GLuint irradianceMap = 0;       // Low-resolution irradiance map for diffuse lighting
	GLuint envCubemap = 0;          // Original environment cubemap
	GLuint skyTextureId = 0;        // Sky texture ID for background rendering
};

class Renderer {
public:
	/**
	 * Initializes the renderer with specified viewport dimensions.
	 * Setup opengl capabilities, create shaders and IBL-dedicated geometry
	 * @param width  The width of the viewport.
	 * @param height The height of the viewport.
	 */
	void init(int width, int height);

	/**
	 * Renders a list of meshes with a given camera.
	 * @param meshes A vector of Mesh objects to be rendered.
	 * @param opaqueMeshesIndices indices of the opaque meshes
	 * @param transparentMeshesIndices indices of the transparent meshes
	 * @param camera The Camera providing the view and projection matrices.
	 */
	void render(const std::vector<Mesh>& meshes, const std::vector<int>& opaqueMeshesIndices, const std::vector<int>& transparentMeshesIndices, const Camera& camera);

	/**
	 * Loads environment maps for IBL from the specified file.
	 * @param filepath The path to the HDR or EXR environment map file.
	 * @see https://learnopengl.com/PBR/IBL/Specular-IBL
	 */
	void loadEnvironment(const std::string& filepath);

	/**
	 * Loads texture data into the GPU based on a TextureBindingEvent.
	 * Create texture and update texture in material
	 * @param tbe The TextureBindingEvent containing texture information.
	 */
	void loadTextureData(const TextureBindingEvent& tbe);

	/**
	 * Loads a single mesh into GPU memory.
	 * @param mesh The Mesh object to load.
	 */
	void loadMesh(Mesh& mesh);

	/**
	 * Loads multiple meshes into GPU memory.
	 * @param meshes A vector of Meshes objects to load.
	 */
	void loadMeshes(std::vector<Mesh>& meshes);

	/**
	 * Clears mesh data from GPU memory for the provided list of meshes.
	 * @param meshes A vector of Mesh objects to clear.
	 */
	void clearMeshes(const std::vector<Mesh>& meshes);

	/**
	 * Clears textures associated with the given materials from GPU memory.
	 * @param materials A vector of Material objects whose textures will be cleared.
	 */
	void clearTextures(const std::vector<Material>& materials);

	/**
	 * Sets the rendering mode (PBR, albedo...)
	 * @param displayMode An integer representing the display mode.
	 */
	void setRenderMode(int displayMode);

	/**
	 * Set the background visibility state
	 * @param bool true to show background
	 */
	void setShowBackground(bool showBackground);

	/**
	 * Set the intensity of the environment
	 * @param envIntensity intensity to set
	 */
	void setEnvIntensity(float envIntensity) { _envIntensity = envIntensity; }

	/**
	 * Resizes the viewport to new dimensions.
	 * @param ivec2 A glm::ivec2 specifying the new width and height.
	 */
	void resizeViewport(const glm::vec2& vec2);

private:
	// Environment maps for IBL
	Environment _environment;
	// Basic geometry for screen-space quad and skybox cube
	Mesh _quadMesh, _cubeMesh;
	// Shaders for PBR and background rendering
	Shader _pbrShader, _backgroundShader;
	// Current viewport dimensions
	int _width, _height;
	// Current render mode
	int _renderMode;
	// Shaders for IBL computing
	Shader _equirectangularToCubemapShader;
	Shader _prefilterShader;
	Shader _irradianceShader;
	Shader _brdfShader;
	// render background or solid color
	bool _showBackground = true;
	float _envIntensity = 1.0f;

	/**
	 * Render a set of meshes
	 * @param meshes the vector of meshes
	 * @param meshIndices indices of meshes to render from the meshes vector
	 */
	void renderMeshes(const std::vector<Mesh>& meshes, const std::vector<int>& meshIndices);

	/**
	 * Sort transparent mesh indices to render from back to front according to camera position
	 * @param meshes the vector of meshes to render
	 * @param transparentMeshIndices the indices of transparent meshes
	 */
	std::vector<int> getSortedTransparentMeshIndices(const std::vector<Mesh>& meshes, const std::vector<int>& transparentMeshIndices, const glm::vec3& cameraPosition);

	/**
	 * Generates a cube mesh for the skybox and generating IBL maps
	 */
	void genCube();

	/**
	 * Generates a quad mesh for full-screen passes
	 */
	void genQuad();

	/**
	 * Renders the cube mesh (used for skybox/environment mapping)
	 */
	void renderCube();

	/**
	 *	Renders the quad mesh (used for screen-space effects)
	 */
	void renderQuad();

	/**
	 * Flips an EXR image vertically to correct orientation.
	 * @param pixels A vector of Imf::Rgba representing the image pixels.
	 * @param width  The width of the image.
	 * @param height The height of the image.
	 */
	void flipImageVertically(std::vector<Imf::Rgba>& pixels, int width, int height);

	/**
	 * Loads an EXR image and returns the OpenGL texture ID.
	 * @param filename The file path to the EXR image.
	 * @return The GLuint ID of the loaded texture.
	 */
	GLuint loadExrImage(const std::string& filename);

	/**
	 * Loads a HDR image and returns the OpenGL texture ID.
	 * @param filename The file path to the HDR image.
	 * @return The GLuint ID of the loaded texture.
	 */
	GLuint loadImage(const std::string& filepath);
};

