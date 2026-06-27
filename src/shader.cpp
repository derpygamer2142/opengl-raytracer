#include "shader.h"


Shader::Shader(const char* path, unsigned int type) {
    this->shaderType = type;
    this->filePath = path;


    const char* shaderSource = readFile(path);

    this->shaderid = glCreateShader(type);
    glShaderSource(this->shaderid, 1, &shaderSource, NULL);
    glCompileShader(this->shaderid);
    
    int success;
    char infoLog[512];
    glGetShaderiv(this->shaderid, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(this->shaderid, 512, NULL, infoLog);
        std::cout << "ERROR: Failed to compile " << shaderTypeAsText() << " from path " << path << ".\n" << infoLog << std::endl;
    }
    
}

const char* Shader::shaderTypeAsText() {
    return this->shaderTypeAsText(this->shaderType);
}

const char* Shader::shaderTypeAsText(int type) {
    switch (type) {
        case GL_FRAGMENT_SHADER: return "FRAGMENT SHADER";
        case GL_VERTEX_SHADER:   return "VERTEX SHADER";
        case GL_GEOMETRY_SHADER: return "GEOMETRY SHADER";
        // OpenGL 3.3 doesn't have tesselation or compute. Might add later
        default: return "INVALID SHADER";
    }
}
    



Program::Program() {
    this->programid = glCreateProgram();
}

void Program::attachShader(Shader shader) {
    this->shaders.push_back(shader);
    glAttachShader(this->programid, shader.shaderid);
}

void Program::link() {
    glLinkProgram(this->programid);
}

void Program::use() {
    glUseProgram(this->programid);
}

unsigned int Program::uniformLocation(const char* uniform) {
    try {
        return this->uniforms.at(uniform);
    }
    catch (std::out_of_range) {
        unsigned int location = glGetUniformLocation(this->programid, uniform);
        // This is very strange.
        this->uniforms.insert(std::pair<const char*, unsigned int>(uniform, location));
        return location;
    }
}

void Program::setUniform1f(const char* uniform, float value) {
    glUniform1f(this->uniformLocation(uniform), value);
}

void Program::setUniform2f(const char* uniform, float v0, float v1) {
    glUniform2f(this->uniformLocation(uniform), v0, v1);
}

void Program::setUniform3f(const char* uniform, float v0, float v1, float v2) {
    glUniform3f(this->uniformLocation(uniform), v0, v1, v2);
}

void Program::setUniform4f(const char* uniform, float v0, float v1, float v2, float v3) {
    glUniform4f(this->uniformLocation(uniform), v0, v1, v2, v3);
}
