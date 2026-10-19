#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <stb_image.h>
#include <filesystem>
#include <cstring>

// Stupid executable directory finding nonsense.
#ifdef _WIN32
	#include <libloaderapi.h>
#elifdef __linux__
	#include <libgen.h>
	#include <linux/limits.h>
#endif


#include "fileutil.h"
#include "shader.h"
#include "modelloader.h"

using namespace std;

float windowWidth = 800.0f;
float windowHeight = 600.0f;
float frameNumber = 1.0f;

// All of the below must match fragment.glsl!
#define MAX_VERTICES 2048
#define MAX_INDICES 4096
#define MAX_MATERIALS 128
#define MAX_LIGHTS 1024
#define MAX_NOISE 1024

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
	windowWidth = static_cast<float>(width);
	windowHeight = static_cast<float>(height);
}

void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);
}

unique_ptr<filesystem::path> getExecutablePath(void* main) {
	#ifdef _WIN32
		//std::cout << "WINDOWS!!!!!!!!!!!!!!!!!" << std::endl;
		char path[1024];
		GetModuleFileNameA(0, path, 1023);
		return make_unique<filesystem::path>(filesystem::path(path).parent_path());
	#elifdef __linux__
		// from https://stackoverflow.com/a/23943306

		filesystem::path result = filesystem::read_symlink("/proc/self/exe") ;
		//std::cout << "AAAA " << result.parent_path() << std::endl;
		return make_unique<filesystem::path>(result.parent_path());
		
		
		string p = "ERROR";
		return make_unique<filesystem::path>(p);
	#else
		string p = "Pound sand.";
		return make_unique<filesystem::path>(p);
	#endif
}

int main() {
	unique_ptr<filesystem::path> CWD = getExecutablePath((void*) main);
	
	Model testModel = loadOBJ(*CWD.get() / "assets/cornellbox.obj");
	// testModel.scale(25.0);
	// testModel.translate(0.0, 0.0, 0.0);
	testModel.transform();
	float* testModelVertexBuffer;
	unsigned int testModelVertSize = testModel.genVertexBuffer(&testModelVertexBuffer);
	float* testModelMaterialBuffer;
	unsigned int testModelMatSize  = testModel.genMaterialBuffer(&testModelMaterialBuffer);
	std::cout << "#3" << std::endl;
	std::cout << "Vertices: " << testModel.final_vertices.size() << std::endl;
	std::cout << "Indices: "  << testModel.triangles.size() << std::endl;
	/*for (float p : testModel.final_vertices) {
		std::cout << "V: " << p << std::endl;
	}
	for (float i : testModel.triangles) {
		std::cout << "Index: " << i << std::endl;
	}*/


	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

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


	Shader vertexShader(*CWD.get() / "shaders/vertex.glsl", GL_VERTEX_SHADER);
	Shader fragmentShader(*CWD.get() / "shaders/fragment.glsl", GL_FRAGMENT_SHADER);
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

	Texture skibidiToilet(*CWD.get() / "assets/skibidi.jpg", 0);
	Framebuffer framebuffer0(static_cast<int>(windowWidth), static_cast<int>(windowHeight), 0, 0);
	Framebuffer framebuffer1(static_cast<int>(windowWidth), static_cast<int>(windowHeight), 1, 1);
	//shaderProgram.setFrameBuffers({ framebuffer0, framebuffer1 });
	
	Shader copyShader(*CWD.get() / "shaders/textureoutput.glsl", GL_FRAGMENT_SHADER);
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
	
	unsigned int modelData;
	glGenBuffers(1, &modelData);
	glBindBuffer(GL_UNIFORM_BUFFER, modelData);
	// std430
	// vec4 vertices[n0] -> n0*4*4
	// uint indices[n1]  -> n1*4
	// uint numTriangles -> 4
	// PADDING -> Variable size depending on size of indices. See below.
	// Material materials[n2] -> n2*48
	// uint lights[n3] -> n3*4
	// uint numLights -> 4
	// uint noise[n4] -> n4*4

	int padding0 = 4*(3-(MAX_INDICES%4)); // Aligned to vec4


	glBufferData(GL_UNIFORM_BUFFER, MAX_VERTICES*4*4 + MAX_INDICES*4 + 4 + padding0 + MAX_MATERIALS*48 + MAX_LIGHTS*4 + 4, NULL, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	shaderProgram.setBlockBindPoint("ModelData", 0);
	shaderProgram.bindBufferToBlock(modelData, 0);

	unsigned int noise[MAX_NOISE];
	srand(time(NULL));

	bool swap = false;

	while (!glfwWindowShouldClose(window)) {
		processInput(window);
		
		shaderProgram.setRenderTarget(swap ? framebuffer1 : framebuffer0);

		//glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

		shaderProgram.use();
		shaderProgram.setUniform1f("iTime", glfwGetTime());
		shaderProgram.setUniform2f("iResolution", windowWidth, windowHeight);
		shaderProgram.setUniform1f("frameNumber", frameNumber);
		//skibidiToilet.bindToSlot(0);
		// framebuffer1.bindToTextureSlot(0);
		shaderProgram.setUniform1i("inputTexture", swap ? 0 : 1);
		//shaderProgram.setUniform3fv("vertices", testModel.final_vertices);
		//shaderProgram.setUniform1uiv("indices", testModel.triangles);
		//shaderProgram.setUniform1ui("numTriangles", static_cast<unsigned int>(testModel.triangles.size()/3u));

		for (int i = 0; i < MAX_NOISE; i++) {
			// Fill the noise buffer
			noise[i] = static_cast<unsigned int>(rand());
		}

		glBindBuffer(GL_UNIFORM_BUFFER, modelData);
		glBufferSubData(GL_UNIFORM_BUFFER, 0, testModelVertSize*4, testModelVertexBuffer);
		//std::cout << "Triangles: " << testModel.triangles.size() << std::endl;
		glBufferSubData(GL_UNIFORM_BUFFER, MAX_VERTICES*4*4, static_cast<unsigned int>(testModel.triangles.size())*4u, testModel.triangles.data());
		unsigned int numTriangles = static_cast<unsigned int>(testModel.triangles.size()/3u);
		// std::cout << numTriangles << std::endl;
		glBufferSubData(GL_UNIFORM_BUFFER, MAX_VERTICES*4*4 + MAX_INDICES*4, 4, &numTriangles);
		glBufferSubData(GL_UNIFORM_BUFFER, MAX_VERTICES*4*4 + MAX_INDICES*4 + 4 + padding0, testModelMatSize*4, testModelMaterialBuffer);
		glBufferSubData(GL_UNIFORM_BUFFER, MAX_VERTICES*4*4 + MAX_INDICES*4 + 4 + padding0 + MAX_MATERIALS*4, static_cast<unsigned int>(testModel.lights.size())*4u, testModel.lights.data());
		unsigned int numLights = static_cast<unsigned int>(testModel.lights.size())/3u;
		glBufferSubData(GL_UNIFORM_BUFFER, MAX_VERTICES*4*4 + MAX_INDICES*4 + 4 + padding0 + MAX_MATERIALS*4 + MAX_LIGHTS*4, 4u, &numLights);
		// glBufferSubData(GL_UNIFORM_BUFFER, MAX_VERTICES*4*4 + MAX_INDICES*4 + 4 + padding0 + MAX_MATERIALS*4 + MAX_LIGHTS*4 + 4u, MAX_NOISE*4u, noise);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		//std::cout << "Triangle data: " << testModel.final_vertices.size() << ", " << testModel.triangles.size() << ", " << static_cast<unsigned int>(testModel.triangles.size()/3u) << std::endl;
		//shaderProgram.setUniformTexture("inputTexture", skibidiToilet);
		
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		


		secondaryProgram.resetRenderTarget(static_cast<int>(windowWidth), static_cast<int>(windowHeight));
		//glClear(GL_COLOR_BUFFER_BIT);

		secondaryProgram.use();
		// framebuffer0.bindToTextureSlot(0);
		secondaryProgram.setUniform1i("inputTexture", swap ? 0 : 1);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);

		glfwSwapBuffers(window);
		//swapFramebuffers(framebuffer0, framebuffer1);
		glfwPollEvents();
		frameNumber += 1.0f;
		swap = !swap;
	}

	glfwTerminate();
	return 0;
}