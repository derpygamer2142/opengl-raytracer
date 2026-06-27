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
   


Texture::Texture(const char* path, unsigned int slot) {
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(path, &this->width, &this->height, &this->numChannels, 0);
	glGenTextures(1, &this->textureid);
	
    this->bindToSlot(slot);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, this->numChannels == 3 ? GL_RGB : GL_RGBA, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(data);
}

void Texture::bindToSlot(unsigned int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, this->textureid);
    this->textureSlot = slot;
}

Framebuffer::Framebuffer(int width, int height, unsigned int slot) {
	glGenFramebuffers(1, &this->frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	
	glGenTextures(1, &this->renderTexture);
	glBindTexture(GL_TEXTURE_2D, this->renderTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + slot, renderTexture, 0);
    
    this->attachmentSlot = slot;
	// unsigned int drawBuffers[1] = { GL_COLOR_ATTACHMENT0 + slot };
	// glDrawBuffers(1, drawBuffers);
	
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		std::cout << "Error with framebuffer" << std::endl;
	}
}

void Framebuffer::bindToTextureSlot(unsigned int slot) {
    glActiveTexture(GL_TEXTURE0 + slot);
	glBindTexture(GL_TEXTURE_2D, this->renderTexture);
    this->textureSlot = slot;
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

// Change the bound outputs of the program
void Program::setFrameBuffers(vector<Framebuffer> targets) {
    // Note to self this sets GL_FRAMEBUFFER and binding the framebuffer sets what is used for rendering
    unsigned int* drawBuffers = new unsigned int[targets.size()]();
    
    for (int i = 0; i < targets.size(); i++) {
        drawBuffers[i] = GL_COLOR_ATTACHMENT0 + targets[i].attachmentSlot;
    }

    glDrawBuffers(targets.size(), drawBuffers);
}

void Program::setRenderTarget(Framebuffer target) {
    glBindFramebuffer(GL_FRAMEBUFFER, target.frameBuffer);
    glViewport(0, 0, target.width, target.height);
}
void Program::resetRenderTarget(int width, int height) {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);
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

void Program::setUniform1i(const char* uniform, int v0) {
    glUniform1i(this->uniformLocation(uniform), v0);
}

void Program::setUniformTexture(const char* uniform, Texture tex) {
    glUniform1i(this->uniformLocation(uniform), tex.textureSlot);
}

void Program::setUniformTexture(const char* uniform, Framebuffer fb) {
    glUniform1i(this->uniformLocation(uniform), fb.textureSlot); // Todo: FIX!
}