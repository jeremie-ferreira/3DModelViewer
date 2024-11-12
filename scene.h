#pragma once
#include "camera.h"
#include "shader.h"
#include "mesh.h"
#include "event.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glad/glad.h>

/**
 * The Scene class manages all objects, materials, and camera setup required for rendering a 3D scene.
 * It loads model data, initializes scene objects, and provides access to meshes and materials.
 */
class Scene {
public:
    /**
     * Initializes the scene with an event bus for communication and sets the aspect ratio for the camera.
     * @param eventBus Pointer to the EventBus for handling events.
     * @param ratio The aspect ratio for rendering (typically screen width / height).
     */
    void init(EventBus* eventBus, float ratio);

    /**
     * Loads a GLB model from the specified file path, processing its meshes and materials.
     * @param filepath Path to the GLB file to load.
     */
    void loadGlb(std::string filepath);

    /**
     * Provides access to the scene's meshes.
     * @return A reference to the vector of Mesh objects in the scene.
     */
    std::vector<Mesh>& getMeshes() { return _meshes; }

    std::vector<int>& getOpaqueMeshes() { return _opaqueMeshes; }

    std::vector<int>& getTransparentMeshes() { return _transparentMeshes; }

    /**
     * Provides access to the scene's materials.
     * @return A reference to the vector of Material objects in the scene.
     */
    std::vector<Material>& getMaterials() { return _materials; }

    //The camera used for viewing the scene.
    Camera camera;

private:
    /**
     * Processes and attaches a texture to a material from a given Assimp material type.
     * @param scene Pointer to the Assimp scene structure containing the model data.
     * @param mat Pointer to the Assimp material to process.
     * @param type The Assimp texture type (e.g., diffuse, specular).
     * @param textureType Custom type to categorize textures within the application.
     * @param material Reference to the Material object where the texture will be stored.
     */
    void processTexture(const aiScene* scene, aiMaterial* mat, aiTextureType type, TextureType textureType, Material& material);

    /**
     *
     */
    void processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, glm::vec3& min, glm::vec3& max);

    /**
     * Processes all materials from the loaded Assimp scene and converts them to application-specific Material objects.
     * @param scene Pointer to the Assimp scene containing model data.
     * @return A vector of Material objects representing the processed materials.
     */
    std::vector<Material> processMaterials(const aiScene* scene);

    // All the meshes in the scene.
    std::vector<Mesh> _meshes;

    // Meshes using an opaque material
    std::vector<int> _opaqueMeshes;

    // Meshes using a transparent material
    std::vector<int> _transparentMeshes;

    // All the materials in the scene.
    std::vector<Material> _materials;

    // Pointer to the EventBus for managing and dispatching events within the scene.
    EventBus* _eventBus;
};
