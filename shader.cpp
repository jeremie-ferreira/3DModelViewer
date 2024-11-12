#include "shader.h"
#include <glm/glm.hpp>

GLuint Shader::compileShader(GLuint type, const std::string& source) {
    GLuint shaderObject = glCreateShader(type);

    const char* src = source.c_str();
    glShaderSource(shaderObject, 1, &src, nullptr);

    glCompileShader(shaderObject);

    GLint success;
    glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &success);

    if (!success) {
        int length;
        glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
        char infoLog[512];
        glGetShaderInfoLog(shaderObject, 512, nullptr, infoLog);
        std::cerr << "Error compiling shader" << std::endl;
        std::cerr << infoLog << std::endl;
        exit(1);
    }

    return shaderObject;
}

GLuint Shader::createShaderProgram(const std::string& vertexShaderSource, const std::string& fragmentShaderSource) {
    GLuint programObject = glCreateProgram();
    GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    glAttachShader(programObject, vertexShader);
    glAttachShader(programObject, fragmentShader);
    glLinkProgram(programObject);

    glValidateProgram(programObject);

    GLint success;
    glGetProgramiv(programObject, GL_LINK_STATUS, &success);

    if (!success) {
        int length;
        glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
        char infoLog[512];
        glGetProgramInfoLog(programObject, 512, nullptr, infoLog);
        std::cerr << "Error linking program object" << std::endl;
        std::cerr << infoLog << std::endl;
        exit(1);
    }

    return programObject;
}

Shader::Shader(const char* vertexPath, const char* fragmentPath) {
    std::string vertexCode;
    std::string fragmentCode;
    std::string geometryCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;
    std::ifstream gShaderFile;
    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    try
    {
        // open files
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        // read file's buffer contents into streams
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        // close file handlers
        vShaderFile.close();
        fShaderFile.close();
        // convert stream into string
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
    }
    catch (std::ifstream::failure& e)
    {
        std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
    }
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();
    // 2. compile shaders
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, vertexPath, false);
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, fragmentPath, false);
    // shader Program
    id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    std::string path = std::string("PROGRAM ") + std::string(vertexPath) + " " + std::string(fragmentPath);
    checkCompileErrors(id, path.c_str(), true);
    // delete the shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertex);
    glDeleteShader(fragment);

}

void Shader::checkCompileErrors(GLuint shader, const char* filename, bool program) {
    GLint success;
    GLchar infoLog[512];

    if (program) {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "ERROR LINKING PROGRAM:\n" << filename << "\n" << infoLog << "\n";
            exit(1);
        }
    }
    else {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 512, nullptr, infoLog);
            std::cerr << "ERROR COMPILING SHADER:\n" << filename << "\n" << infoLog << "\n";
            exit(1);
        }
    }
}

void Shader::use() {
    glUseProgram(id);
}

void Shader::setInt(const char* name, int value) {
    glUniform1i(glGetUniformLocation(id, name), value);
}

void Shader::setFloat(const char* name, float value) {
    glUniform1f(glGetUniformLocation(id, name), value);
}

void Shader::setVec2(const char* name, glm::vec2 value) {
    glUniform2fv(glGetUniformLocation(id, name), 1, &value[0]);
}

void Shader::setVec3(const char* name, glm::vec3 value) {
    glUniform3fv(glGetUniformLocation(id, name), 1, &value[0]);
}

void Shader::setVec4(const char* name, glm::vec4 value) {
    glUniform4fv(glGetUniformLocation(id, name), 1, &value[0]);
}

void Shader::setMat4(const char* name, glm::mat4 value) {
    glUniformMatrix4fv(glGetUniformLocation(id, name), 1, GL_FALSE, &value[0][0]);
}