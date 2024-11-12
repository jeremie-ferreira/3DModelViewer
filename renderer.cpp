#include "renderer.h"
#include "shader.h"
#include <glad/glad.h>
#include <ImfRgbaFile.h>
#include <ImfRgba.h>
#include <ImfArray.h>
#include <stb_image.h>
#include <filesystem>

static void glClearAllErrors() {
    while (glGetError() != GL_NO_ERROR) {}
}

static bool glCheckErrorStatus(const char* function, int line) {
    while (GLenum error = glGetError()) {
        std::cout << "OPENGL ERROR FUNCTION " << function << "LINE " << line << ": " << error << "\n";
        exit(1);
    }
    return false;
}

#define glCheck(x) glClearAllErrors(); x; glCheckErrorStatus(#x, __LINE__);

void Renderer::init(int width, int height) {
    _width = width;
    _height = height;

    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    // set depth function to less than AND equal for skybox depth trick.
    //glDepthFunc(GL_LEQUAL);
    // enable seamless cubemap sampling for lower mip levels in the pre-filter map.
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Compile Shaders
    _pbrShader = Shader("./shaders/pbr.vs", "./shaders/pbr.fs");
    _backgroundShader = Shader("./shaders/background.vs", "./shaders/background.fs");
    _equirectangularToCubemapShader = Shader("./shaders/cubemap.vs", "./shaders/equirectangular_to_cubemap.fs");
    _prefilterShader = Shader("./shaders/cubemap.vs", "./shaders/prefilter.fs");
    _irradianceShader = Shader("./shaders/cubemap.vs", "./shaders/irradiance_convolution.fs");
    _brdfShader = Shader("./shaders/brdf.vs", "./shaders/brdf.fs");
    // Generate Quad and Cube meshes
    genCube();
    genQuad();
}

std::vector<int> Renderer::getSortedTransparentMeshIndices(const std::vector<Mesh>& meshes, const std::vector<int>& transparentMeshIndices, const glm::vec3& cameraPosition) {
    // Vector to store sorted indices
    std::vector<int> sortedIndices = transparentMeshIndices;

    // Sort the indices based on the distance of the mesh from the camera
    std::sort(sortedIndices.begin(), sortedIndices.end(), [&meshes, &cameraPosition](int a, int b) {
        // Extract world position from the mesh's transform matrix (translation part)
        glm::vec3 posA = glm::vec3(meshes[a].transform[3][0], meshes[a].transform[3][1], meshes[a].transform[3][2]);
        glm::vec3 posB = glm::vec3(meshes[b].transform[3][0], meshes[b].transform[3][1], meshes[b].transform[3][2]);

        // Calculate the distance between the mesh and the camera
        float distA = glm::length(posA - cameraPosition);
        float distB = glm::length(posB - cameraPosition);

        // Sort in descending order (farthest to closest)
        return distA > distB;
        });

    return sortedIndices;
}

void Renderer::renderMeshes(const std::vector<Mesh>& meshes, const std::vector<int>& meshIndices) {
    for (int index : meshIndices) {
        const Mesh& mesh = meshes[index];

        // diffuse map
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, mesh.material.diffuse);
        _pbrShader.setInt("uAlbedoMap", 1);
        // normal map
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, mesh.material.normal);
        _pbrShader.setInt("uNormalMap", 2);
        // metal roughness map
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, mesh.material.metalnessRoughness);
        _pbrShader.setInt("uMetalnessRoughnessMap", 3);

        _pbrShader.setInt("uUseNormalMap", mesh.material.normal);
        _pbrShader.setFloat("uMetalnessFactor", mesh.material.metalnessFactor);
        _pbrShader.setFloat("uRoughnessFactor", mesh.material.roughnessFactor);
        _pbrShader.setVec4("uDiffuseColor", mesh.material.diffuseColor);

        // bind buffers
        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);

        // mesh uniforms
        _pbrShader.setMat4("uModel", mesh.transform);

        // draw mesh
        glDrawElements(GL_TRIANGLES, mesh.indices.size(), GL_UNSIGNED_INT, 0);
    }
}

void Renderer::render(const std::vector<Mesh>& meshes, const std::vector<int>& opaqueMeshesIndices, const std::vector<int>& transparentMeshesIndices, const Camera& camera) {
    // set viewport
    glViewport(0, 0, _width, _height);

    // clear buffers
    glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // render background
    // =================
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_FALSE);
    glDisable(GL_DEPTH_TEST);
    

    if (_showBackground) {
        // configure background shader
        _backgroundShader.use();

        // environment map
        glActiveTexture(GL_TEXTURE0);
        _backgroundShader.setInt("environmentMap", 0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, _environment.envCubemap);

        // pass uniforms
        _backgroundShader.setMat4("view", camera.getTransform());
        _backgroundShader.setMat4("projection", camera.getPerspective());

        // render cube
        renderCube();
    }

    // setup light
    glm::vec3 lightDirection = glm::normalize(glm::vec3(-.5, -.5, -1));
    glm::vec3 lightColor(1, 1, 1);
   
    // configure PBR Shader
    // --------------------
    _pbrShader.use();
    _pbrShader.setInt("uRenderMode", _renderMode);

    // prefilter map
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _environment.prefilterMap);
    _pbrShader.setInt("uPrefilterMap", 0);
    // environment map
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_2D, _environment.brdfLutTexture);
    _pbrShader.setInt("uBrdfLut", 4);
    // irradiance map
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _environment.irradianceMap);
    _pbrShader.setInt("uIrradianceMap", 5);

    // global uniforms
    _pbrShader.setVec3("uLightDirection", lightDirection);
    _pbrShader.setVec3("uLightColor", lightColor);
    _pbrShader.setFloat("uEnvIntensity", _envIntensity);
    _pbrShader.setMat4("uProjection", camera.getPerspective());
    _pbrShader.setMat4("uView", camera.getTransform());
    _pbrShader.setVec3("uViewPosition", camera.getPosition());

    // Opaque pass
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    
    renderMeshes(meshes, opaqueMeshesIndices);

    // Transparent pass
    // ----------------
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);
    std::vector<int> sortedTransparentIndices = getSortedTransparentMeshIndices(meshes, transparentMeshesIndices, camera.getPosition());
    renderMeshes(meshes, sortedTransparentIndices);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void Renderer::loadEnvironment(const std::string& filepath) {
    // pbr: load the HDR environment map
    // ---------------------------------
    GLuint hdrTexture = 0;
    size_t dotPosition = filepath.find_last_of(".");
    if (dotPosition != std::string::npos && dotPosition == 0) {
        return;
    }
    std::string extension = filepath.substr(dotPosition + 1);
    if (extension == "exr") {
        hdrTexture = loadExrImage(filepath);
    }
    else if (extension == "hdr") {
        hdrTexture = loadImage(filepath);
    }

    // delete previous textures
    if (_environment.prefilterMap > 0) glDeleteTextures(1, &_environment.prefilterMap);
    if (_environment.brdfLutTexture > 0) glDeleteTextures(1, &_environment.brdfLutTexture);
    if (_environment.irradianceMap > 0) glDeleteTextures(1, &_environment.irradianceMap);
    if (_environment.envCubemap > 0) glDeleteTextures(1, &_environment.envCubemap);
    if (_environment.skyTextureId > 0) glDeleteTextures(1, &_environment.skyTextureId);

    // pbr: setup framebuffer
    // ----------------------
    unsigned int captureFBO;
    unsigned int captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    // pbr: setup cubemap to render to and attach to framebuffer
    // ---------------------------------------------------------
    glGenTextures(1, &_environment.envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _environment.envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // enable pre-filter mipmap sampling (combatting visible dots artifact)
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // pbr: set up projection and view matrices for capturing data onto the 6 cubemap face directions
    // ----------------------------------------------------------------------------------------------
    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
    {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    // pbr: convert HDR equirectangular environment map to cubemap equivalent
    // ----------------------------------------------------------------------
    _equirectangularToCubemapShader.use();
    _equirectangularToCubemapShader.setInt("equirectangularMap", 0);
    _equirectangularToCubemapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, 512, 512); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        _equirectangularToCubemapShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, _environment.envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // then let OpenGL generate mipmaps from first mip face (combatting visible dots artifact)
    glBindTexture(GL_TEXTURE_CUBE_MAP, _environment.envCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: create an irradiance cubemap, and re-scale capture FBO to irradiance scale.
    // --------------------------------------------------------------------------------
    glGenTextures(1, &_environment.irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _environment.irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    // pbr: solve diffuse integral by convolution to create an irradiance (cube)map.
    // -----------------------------------------------------------------------------
    _irradianceShader.use();
    _irradianceShader.setInt("environmentMap", 0);
    _irradianceShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _environment.envCubemap);

    glViewport(0, 0, 32, 32); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        _irradianceShader.setMat4("view", captureViews[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, _environment.irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        renderCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: create a pre-filter cubemap, and re-scale capture FBO to pre-filter scale.
    // --------------------------------------------------------------------------------
    int hires = 1024;
    glGenTextures(1, &_environment.prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _environment.prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, hires, hires, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // be sure to set minification filter to mip_linear 
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // generate mipmaps for the cubemap so OpenGL automatically allocates the required memory.
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // pbr: run a quasi monte-carlo simulation on the environment lighting to create a prefilter (cube)map.
    // ----------------------------------------------------------------------------------------------------
    _prefilterShader.use();
    _prefilterShader.setInt("environmentMap", 0);
    _prefilterShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _environment.envCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    unsigned int maxMipLevels = 5;

    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = static_cast<unsigned int>(hires * std::pow(0.5, mip));
        unsigned int mipHeight = static_cast<unsigned int>(hires * std::pow(0.5, mip));
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        _prefilterShader.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            _prefilterShader.setMat4("view", captureViews[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, _environment.prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    glGenTextures(1, &_environment.brdfLutTexture);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, _environment.brdfLutTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _environment.brdfLutTexture, 0);

    glViewport(0, 0, 512, 512);
    _brdfShader.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // delete framebuffers
    glDeleteFramebuffers(1, &captureFBO);
    glDeleteRenderbuffers(1, &captureRBO);
}

void Renderer::loadTextureData(const TextureBindingEvent& tbe) {
    // Determine the image format
    GLenum format = GL_RGB;
    if (tbe.channels == 1) format = GL_RED;
    else if (tbe.channels == 3) format = GL_RGB;
    else if (tbe.channels == 4) format = GL_RGBA;

    // Load texture in GPU
    GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, format, tbe.width, tbe.height, 0, format, GL_UNSIGNED_BYTE, tbe.imageData);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Unbind the texture
    glBindTexture(GL_TEXTURE_2D, 0);

    // update the texture id for the adequate texture of the material
    if (tbe.type == TextureType::MetalnessRoughness) tbe.material->metalnessRoughness = textureId;
    else if (tbe.type == TextureType::Diffuse) tbe.material->diffuse = textureId;
    else if (tbe.type == TextureType::Normal) tbe.material->normal = textureId;
}

void Renderer::resizeViewport(const glm::vec2& vec2) {
    _width = vec2.x;
    _height = vec2.y;
    glViewport(0, 0, _width, _height);
}

void Renderer::loadMesh(Mesh& mesh) {
    // generate vertex array object
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    // generate vertex buffer object
    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, mesh.vertices.size() * sizeof(Vertex), mesh.vertices.data(), GL_STATIC_DRAW);
    
    // generate element buffer object
    glGenBuffers(1, &mesh.ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh.indices.size() * sizeof(GLuint), mesh.indices.data(), GL_STATIC_DRAW);

    // vertex attrib pointers
    // ----------------------
    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // tangent
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
    // uv
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    // unbind vao
    glBindVertexArray(0);
}

void Renderer::loadMeshes(std::vector<Mesh>& meshes) {
    for (Mesh& mesh : meshes) {
        loadMesh(mesh);
    }
}

void Renderer::clearMeshes(const std::vector<Mesh>& meshes) {
    for (Mesh mesh : meshes) {
        // delete vao, vbo and ebo
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
    }
}

void Renderer::clearTextures(const std::vector<Material>& materials) {
    for (Material material : materials) {
        // delete diffuse, normal and metal roughness textures
        glDeleteTextures(1, &material.diffuse);
        glDeleteTextures(1, &material.normal);
        glDeleteTextures(1, &material.metalnessRoughness);
    }
}

void Renderer::setRenderMode(int renderMode) {
    _renderMode = renderMode;
}

void Renderer::setShowBackground(bool showBackground) {
    _showBackground = showBackground;
}


void Renderer::flipImageVertically(std::vector<Imf::Rgba>& pixels, int width, int height) {
    int rowSize = width;  // Number of pixels per row
    for (int y = 0; y < height / 2; ++y) {
        // Calculate indices for the top and bottom rows
        int topIndex = y * rowSize;
        int bottomIndex = (height - y - 1) * rowSize;

        // Swap the rows
        for (int x = 0; x < rowSize; ++x) {
            std::swap(pixels[topIndex + x], pixels[bottomIndex + x]);
        }
    }
}

GLuint Renderer::loadExrImage(const std::string& filename) {
    try {
        // Open the EXR image file specified by 'filename'
        Imf::RgbaInputFile file(filename.c_str());

        // Get the data window of the EXR file, which specifies the valid pixel region
        Imath::Box2i dw = file.dataWindow();

        // Calculate the width and height of the image based on the data window dimensions
        int width = dw.max.x - dw.min.x + 1;
        int height = dw.max.y - dw.min.y + 1;

        // Create a vector to hold pixel data for the entire image
        std::vector<Imf::Rgba> pixels(width * height);

        // Set the framebuffer for the input file, which specifies where to store the pixel data
        file.setFrameBuffer(&pixels[0] - dw.min.x - dw.min.y * width, 1, width);

        // Read the pixel data from the EXR file into the pixel vector
        file.readPixels(dw.min.y, dw.max.y);

        // flip image for GPU
        flipImageVertically(pixels, width, height);

        // Convert the pixel data to OpenGL format and create the texture
        GLuint exrTextureId;
        glGenTextures(1, &exrTextureId);
        glBindTexture(GL_TEXTURE_2D, exrTextureId);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, &pixels[0]);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        return exrTextureId;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load EXR: " << e.what() << std::endl;
        exit(-1);
    }
}

GLuint Renderer::loadImage(const std::string& filepath) {
    int width, height, channels;

    stbi_set_flip_vertically_on_load(true);
    float* data = stbi_loadf(filepath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        std::cerr << "Failed to load image from memory: " << stbi_failure_reason() << std::endl;
        exit(-1);
    }

    // Convert the pixel data to OpenGL format and create the texture
    GLuint hdrTextureId;
    glGenTextures(1, &hdrTextureId);
    glBindTexture(GL_TEXTURE_2D, hdrTextureId);

    GLenum format = (channels == 3) ? GL_RGB : GL_RGBA;
    GLenum internalFormat = (channels == 3) ? GL_RGB16F : GL_RGBA16F;

    glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, GL_FLOAT, &data[0]);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return hdrTextureId;
}

void Renderer::renderCube() {
    // bind cube vao and draw
    glBindVertexArray(_cubeMesh.vao);
    glCheck(glDrawArrays(GL_TRIANGLES, 0, 36));
    glBindVertexArray(0);
}

void Renderer::genCube() {
    // define vertices
    float vertices[] = {
        // back face
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
         1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
         1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
        -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
        -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
        // front face
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
         1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
        -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
        -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
        // left face
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
        -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
        // right face
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
         1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
         1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
         1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
        // bottom face
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
         1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
         1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
        -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
        -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
        // top face
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
         1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
         1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
         1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
        -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
        -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
    };
    glGenVertexArrays(1, &_cubeMesh.vao);
    glGenBuffers(1, &_cubeMesh.vbo);
    // fill buffer
    glBindBuffer(GL_ARRAY_BUFFER, _cubeMesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // link vertex attributes
    glBindVertexArray(_cubeMesh.vao);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::genQuad() {
    // define vertices
    float quadVertices[] = {
        // positions        // texture Coords
        -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
         1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };
    // setup plane VAO
    glGenVertexArrays(1, &_quadMesh.vao);
    glGenBuffers(1, &_quadMesh.vbo);
    glBindVertexArray(_quadMesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, _quadMesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
}

void Renderer::renderQuad() {
    // bind quad vao and draw
    glBindVertexArray(_quadMesh.vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}