#ifndef SHADER_CLASSES
#define SHADER_CLASSES

#include "fileutil.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <unordered_map>
#include <stb_image.h>


class Shader {
    public:
        int shaderType;
        int shaderid;
        const char* filePath;

        Shader(const char* path, unsigned int type);

        const char* shaderTypeAsText();
    
        static const char* shaderTypeAsText(int type);
    
};


class Texture {
    public:
        int width, height, numChannels;
        unsigned int textureid;
        // Slot from 0-31
        unsigned int textureSlot;

        Texture(const char* path, unsigned int slot);

        // Todo: Bindless?
        void bindToSlot(unsigned int slot);
};

class Framebuffer {
    public:
        unsigned int renderTexture, frameBuffer;
        unsigned int textureSlot,   attachmentSlot;
        int width, height;
        Framebuffer(int width, int height, unsigned int slot, unsigned int texSlot);
        
        void bindToTextureSlot(unsigned int slot);
        // Todo: Resizing
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

        void setFrameBuffers(vector<Framebuffer> targets);

        void setRenderTarget(Framebuffer target);

        // Resets the target to be the screen
        void resetRenderTarget(int width, int height);

        unsigned int uniformLocation(const char* uniform);

        void setUniform1f(const char* uniform, float value);

        void setUniform2f(const char* uniform, float v0, float v1);

        void setUniform3f(const char* uniform, float v0, float v1, float v2);

        void setUniform4f(const char* uniform, float v0, float v1, float v2, float v3);

        void setUniform1i(const char* uniform, int v0);

        void setUniformTexture(const char* uniform, Texture tex);

        void setUniformTexture(const char* uniform, Framebuffer fb);
};

#endif