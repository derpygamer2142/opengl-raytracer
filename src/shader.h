#ifndef SHADER_CLASSES
#define SHADER_CLASSES

#include "fileutil.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_map>


class Shader {
    public:
        int shaderType;
        int shaderid;
        const char* filePath;

        Shader(const char* path, unsigned int type);

        const char* shaderTypeAsText();
    
        static const char* shaderTypeAsText(int type);
    
};


class Program {
    public:
        unsigned int programid;
        vector<Shader> shaders{};
        unordered_map<const char*, unsigned int> uniforms;

        Program();

        void attachShader(Shader shader);

        void link();

        void use();

        unsigned int uniformLocation(const char* uniform);

        void setUniform1f(const char* uniform, float value);

        void setUniform2f(const char* uniform, float v0, float v1);

        void setUniform3f(const char* uniform, float v0, float v1, float v2);

        void setUniform4f(const char* uniform, float v0, float v1, float v2, float v3);

};

#endif