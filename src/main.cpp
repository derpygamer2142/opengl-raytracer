#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <stb_image.h>
#include "fileutil.h"
#include "shader.h"
using namespace std;

float windowWidth = 800.0f;
float windowHeight = 600.0f;
float frameNumber = 1.0f;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowWidth = static_cast<float>(width);
	windowHeight = static_cast<float>(height);
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

int main() {

	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	// glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
	// glfwWindowHint(GLFW_CONTEXT_CREATION_API, GLFW_NATIVE_CONTEXT_API);

	GLFWwindow* window = glfwCreateWindow(static_cast<int>(windowWidth), static_cast<int>(windowHeight), "SKIBIDI TOILET!!!!!!!", NULL, NULL);
	if (window == NULL) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cout << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glViewport(0, 0, static_cast<int>(windowWidth), static_cast<int>(windowHeight));
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);


	Shader vertexShader("shaders/vertex.glsl", GL_VERTEX_SHADER);
	Shader fragmentShader("shaders/fragment.glsl", GL_FRAGMENT_SHADER);
	Program shaderProgram;
	shaderProgram.attachShader(vertexShader);
	shaderProgram.attachShader(fragmentShader);
	shaderProgram.link();

	float vertices[] = {
		1.0f, 1.0f, 0.0f,		1.0f, 1.0f,
		1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
		-1.0f, -1.0f, 0.0f, 	0.0f, 0.0f,
		-1.0f, 1.0f, 0.0f,		0.0f, 1.0f
	};
	unsigned int indices[] = {
		0, 1, 3,
		1, 2, 3
	};
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	Texture skibidiToilet("assets/skibidi.jpg", 0);
	Framebuffer testFramebuffer(static_cast<int>(windowWidth), static_cast<int>(windowHeight), 0);
	shaderProgram.setFrameBuffers({ testFramebuffer });
	
	Shader copyShader("shaders/textureoutput.glsl", GL_FRAGMENT_SHADER);
	Program secondaryProgram;
	secondaryProgram.attachShader(vertexShader);
	secondaryProgram.attachShader(copyShader);
	secondaryProgram.link();


	unsigned int VAO, VBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	
	glBindVertexArray(VAO);


	// glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	// glViewport(0, 0, static_cast<int>(windowWidth), static_cast<int>(windowHeight));

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	unsigned int EBO;
	glGenBuffers(1, &EBO);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);


	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(0));
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3*sizeof(float)));
	glEnableVertexAttribArray(1);
	

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		
		shaderProgram.setRenderTarget(testFramebuffer);

		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

		shaderProgram.use();
		shaderProgram.setUniform1f("iTime", glfwGetTime());
		shaderProgram.setUniform2f("iResolution", windowWidth, windowHeight);
		shaderProgram.setUniform1f("frameNumber", frameNumber);
		//skibidiToilet.bindToSlot(0);
		testFramebuffer.bindToTextureSlot(0);
		shaderProgram.setUniform1i("inputTexture", 0);
		//shaderProgram.setUniformTexture("inputTexture", skibidiToilet);
		
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		


		secondaryProgram.resetRenderTarget(static_cast<int>(windowWidth), static_cast<int>(windowHeight));
		//glClear(GL_COLOR_BUFFER_BIT);

		secondaryProgram.use();
		testFramebuffer.bindToTextureSlot(0);
		secondaryProgram.setUniform1i("inputTexture", 0);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);

		glfwSwapBuffers(window);
		glfwPollEvents();
		frameNumber += 1.0f;
	}

	glfwTerminate();
	return 0;
}