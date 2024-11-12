#include "scene.h"
#include <glm/glm.hpp>
#include <iostream>
#include <fstream>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void Scene::init(EventBus* eventBus, float ratio) {
    _eventBus = eventBus;
    camera.init(ratio);
}

void Scene::loadGlb(std::string filepath) {
    // free gpu meshes and textures data
    _eventBus->publish(Event(EventType::ClearGpuMeshesAndTextures));
    _meshes.clear();
    _opaqueMeshes.clear();
    _transparentMeshes.clear();
    _materials.clear();

    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cerr << "Error loading GLB model: " << importer.GetErrorString() << std::endl;
        exit(EXIT_FAILURE);
    }

    _materials = processMaterials(scene);

    // min and max for bounding box processing
    glm::vec3 min(std::numeric_limits<float>::max());
    glm::vec3 max(std::numeric_limits<float>::lowest());

    // Process nodes and meshes recursively
    processNode(scene->mRootNode, scene, glm::mat4(1.0f), min, max);

    // Center object bounding box on world origin
    glm::vec3 center = (min + max) * 0.5f;
    glm::mat4 transform = glm::translate(glm::mat4(1.0f), -center);
    for (Mesh& mesh : _meshes) {
        mesh.transform = transform;
    }

    // load meshes to gpu
    _eventBus->publish(Event(EventType::LoadGpuMeshes));
}

void Scene::processNode(aiNode* node, const aiScene* scene, glm::mat4 parentTransform, glm::vec3& min, glm::vec3& max) {
    // Convert Assimp matrix to GLM matrix
    glm::mat4 nodeTransform = glm::transpose(glm::make_mat4(&node->mTransformation.a1));
    glm::mat4 globalTransform = parentTransform * nodeTransform;

    // Process each mesh in this node
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* assimpMesh = scene->mMeshes[node->mMeshes[i]];
        Mesh mesh;
        mesh.name = assimpMesh->mName.C_Str();

        // vertices
        for (unsigned int j = 0; j < assimpMesh->mNumVertices; ++j) {
            aiVector3D position = assimpMesh->mVertices[j];
            aiVector3D normal = assimpMesh->mNormals[j];
            aiVector3D tangent = assimpMesh->mTangents[j];
            aiVector3D uv = assimpMesh->mTextureCoords[0][j];

            Vertex vertex;
            vertex.position = glm::vec3(globalTransform * glm::vec4(position.x, position.y, position.z, 1.0f));
            vertex.normal = glm::vec3(globalTransform * glm::vec4(normal.x, normal.y, normal.z, 0.0f));
            vertex.uv = glm::vec2(uv.x, uv.y);
            vertex.tangent = glm::vec3(globalTransform * glm::vec4(tangent.x, tangent.y, tangent.z, 0.0f));
            mesh.vertices.push_back(vertex);

            min.x = std::min(min.x, vertex.position.x);
            min.y = std::min(min.y, vertex.position.y);
            min.z = std::min(min.z, vertex.position.z);
            max.x = std::max(max.x, vertex.position.x);
            max.y = std::max(max.y, vertex.position.y);
            max.z = std::max(max.z, vertex.position.z);
        }

        // faces
        for (unsigned int j = 0; j < assimpMesh->mNumFaces; ++j) {
            aiFace face = assimpMesh->mFaces[j];
            mesh.indices.push_back(face.mIndices[0]);
            mesh.indices.push_back(face.mIndices[1]);
            mesh.indices.push_back(face.mIndices[2]);
        }

        mesh.material = _materials[assimpMesh->mMaterialIndex];
        mesh.transform = globalTransform;

        _meshes.push_back(mesh);

        if (mesh.material.diffuseColor.a < 1) {
            _transparentMeshes.push_back(_meshes.size() - 1);
        }
        else {
            _opaqueMeshes.push_back(_meshes.size() - 1);
        }
    }

    // Recurse for each child
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene, globalTransform, min, max);
    }
}

std::vector<Material> Scene::processMaterials(const aiScene* scene) {
    std::vector<Material> sceneMaterials;

    for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
        aiMaterial* mat = scene->mMaterials[i];

        Material material;
        material.name = mat->GetName().C_Str();
        
        processTexture(scene, mat, aiTextureType_DIFFUSE, TextureType::Diffuse, material);
        processTexture(scene, mat, aiTextureType_NORMALS, TextureType::Normal, material);
        processTexture(scene, mat, aiTextureType_METALNESS, TextureType::MetalnessRoughness, material);

        aiColor4D color;
        float value = 1.0f;
        if (material.diffuse == 0 && mat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
            material.diffuseColor = glm::vec4(color.r, color.g, color.b, color.a);
        }
        if (material.metalnessRoughness == 0 && mat->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS) {
            material.metalnessFactor = value;
        }
        if (material.metalnessRoughness == 0 && mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, value) == AI_SUCCESS) {
            material.roughnessFactor = value;
        }

        sceneMaterials.push_back(material);
    }

    return sceneMaterials;
}

void Scene::processTexture(const aiScene* scene, aiMaterial* mat, aiTextureType type, TextureType textureType, Material& material) {
    if (mat->GetTextureCount(type) > 0) {
        aiString texturePath;
        mat->GetTexture(type, 0, &texturePath);

        // If the texture is embedded, it will be prefixed with "*"
        if (texturePath.C_Str()[0] == '*') {
            // Extract texture index
            int textureIndex = atoi(texturePath.C_Str() + 1);
            if (textureIndex >= 0 && textureIndex < scene->mNumTextures) {
                // Get the embedded texture
                const aiTexture* texture = scene->mTextures[textureIndex];

                // Access texture data
                if (texture->mHeight == 0) {
                    // Uncompressed texture in memory, e.g., PNG or JPG

                    int width, height, channels;
                    unsigned char* imageData = nullptr;

                    // If the texture is compressed (e.g., PNG, JPG)
                    if (texture->mHeight == 0) {
                        // stb_image expects raw image data in memory to decode
                        stbi_set_flip_vertically_on_load(false);
                        imageData = stbi_load_from_memory(reinterpret_cast<stbi_uc*>(texture->pcData), texture->mWidth, &width, &height, &channels, 0);
                        if (!imageData) {
                            std::cerr << "Failed to load texture: " << stbi_failure_reason() << std::endl;
                        }
                    }
                    else {
                        // RAW texture data (uncompressed)
                        std::cerr << "Unsupported RAW texture format." << std::endl;
                    }

                    if (imageData) {
                        // load gpu texture
                        _eventBus->publish(Event(EventType::LoadTextureRenderData,
                            TextureBindingEvent(&material, textureType, imageData, channels, width, height)));

                        // After use, don't forget to free the image data
                        stbi_image_free(imageData);
                    }
                }
                else {
                    // RAW format (e.g., uncompressed pixel data)
                    std::cout << "RAW texture data found. Not implemented yet" << std::endl;
                    exit(-1);
                }
            }
        }
    }
}
