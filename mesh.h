#pragma once
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

/**
 * Vertex structure representing a single vertex's attributes for rendering.
 */
struct Vertex {
    glm::vec3 position;  // 3D position of the vertex
    glm::vec3 normal;    // Normal vector at the vertex, used for lighting
    glm::vec3 tangent;   // Tangent vector, used for normal mapping
    glm::vec2 uv;        // Texture coordinates for mapping textures
};

/**
 * Enumeration for different types of textures in a material.
 */
enum class TextureType {
    Diffuse,             // Diffuse color texture
    Normal,              // Normal map for simulating surface detail
    MetalnessRoughness   // Metalness and roughness map for PBR
};

/**
 * Material structure holding texture information and material properties.
 */
struct Material {
    std::string name;            // Name of the material
    GLuint diffuse = 0;              // Texture ID for the diffuse map
    GLuint metalnessRoughness = 0;   // Texture ID for the metalness-roughness map
    GLuint normal = 0;               // Texture ID for the normal map

    glm::vec4 diffuseColor = glm::vec4(2);
    float metalnessFactor = 2;
    float roughnessFactor = 2;
};

/**
 * Mesh class representing a 3D model with vertices, indices, and material.
 */
class Mesh {
public:
    std::string name;                 // Name of the mesh
    std::vector<Vertex> vertices;     // List of vertices in the mesh
    std::vector<GLuint> indices;      // List of indices for indexed rendering
    GLuint diffuseTextureId;          // Texture ID for the diffuse map

    GLuint vao;                       // Vertex Array Object ID
    GLuint vbo;                       // Vertex Buffer Object ID
    GLuint ebo;                       // Element Buffer Object ID

    Material material;                // Material associated with the mesh
    glm::mat4 transform = glm::mat4(1.0f); // Transformation matrix for the mesh
};
