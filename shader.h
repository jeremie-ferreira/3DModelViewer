#pragma once
#include <glad/glad.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <glm/glm.hpp>

/**
 * The Shader class handles compiling, linking, and managing shader programs.
 * It provides utility functions for setting uniform variables in shaders.
 */
class Shader {
public:
    
    // OpenGL ID for the shader program.
    GLuint id;

    /**
     * Default constructor.
     */
    Shader() {}

    /**
     * Initializes and compiles the shader program from vertex and fragment shader file paths.
     * @param vertexPath Path to the vertex shader source file.
     * @param fragmentPath Path to the fragment shader source file.
     */
    Shader(const char* vertexPath, const char* fragmentPath);

    /**
     * Activates the shader program for use in the current OpenGL context.
     */
    void use();

    /**
     * Sets an integer uniform in the shader program.
     * @param name Name of the uniform variable in the shader.
     * @param value Integer value to set.
     */
    void setInt(const char* name, int value);

    /**
     * Sets a float uniform in the shader program.
     * @param name Name of the uniform variable in the shader.
     * @param value Float value to set.
     */
    void setFloat(const char* name, float value);

    /**
     * Sets a 2D vector uniform in the shader program.
     * @param name Name of the uniform variable in the shader.
     * @param value 2D vector (glm::vec2) to set.
     */
    void setVec2(const char* name, glm::vec2 value);

    /**
     * Sets a 3D vector uniform in the shader program.
     * @param name Name of the uniform variable in the shader.
     * @param value 3D vector (glm::vec3) to set.
     */
    void setVec3(const char* name, glm::vec3 value);

    /**
     * Sets a 4D vector uniform in the shader program.
     * @param name Name of the uniform variable in the shader.
     * @param value 4D vector (glm::vec4) to set.
     */
    void setVec4(const char* name, glm::vec4 value);

    /**
     * Sets a 4x4 matrix uniform in the shader program.
     * @param name Name of the uniform variable in the shader.
     * @param value 4x4 matrix (glm::mat4) to set.
     */
    void setMat4(const char* name, glm::mat4 value);

private:
    /**
     * Compiles an individual shader (vertex or fragment) from source.
     * @param type The type of shader (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
     * @param source Shader source code as a string.
     * @return The compiled shader ID.
     */
    GLuint compileShader(GLuint type, const std::string& source);

    /**
     * Creates a shader program by linking compiled vertex and fragment shaders.
     * @param vertexShaderSource Source code for the vertex shader.
     * @param fragmentShaderSource Source code for the fragment shader.
     * @return The linked shader program ID.
     */
    GLuint createShaderProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource);

    /**
     * Checks for compile or linking errors in a shader or shader program.
     * @param shader The shader ID or program ID to check.
     * @param filename The name of the shader file for error context.
     * @param program Boolean indicating whether this is a program (true) or an individual shader (false).
     */
    void checkCompileErrors(GLuint shader, const char* filename, bool program);
};