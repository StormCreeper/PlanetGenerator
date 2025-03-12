#define _USE_MATH_DEFINES

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include <iostream>
#include <string>
#include <vector>
#include <cmath>

#include "ShaderProgram.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Camera.h"

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void generateSphereMesh(int subdivisions, std::vector<glm::vec3>& vertices,
						std::vector<glm::uvec3>& indices);

// Global variables
GLuint VAO, VBO, EBO;
float deltaTime = 0.0f, lastFrame = 0.0f;

std::shared_ptr<Camera> cameraPtr;

static bool isRotating(false);
static bool isPanning(false);
static bool isZooming(false);
static double baseX(0.0), baseY(0.0);
static glm::vec3 baseTrans(0.0);
static glm::vec3 baseRot(0.0);
bool isWireframe = false;

void keyCallback(GLFWwindow* windowPtr, int key, int scancode, int action,
				 int mods) {
	if (action == GLFW_PRESS) {
		if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
			glfwSetWindowShouldClose(windowPtr, true);
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_F12) {
			std::string shaders_folder = "../Resources/Shaders/";
			std::shared_ptr<ShaderProgram> shader =
				ShaderProgram::genBasicShaderProgram(
					shaders_folder + "shader.vert",
					shaders_folder + "shader.frag");
		}
		if (action == GLFW_PRESS && key == GLFW_KEY_W) {
			isWireframe = !isWireframe;
			glPolygonMode(GL_FRONT_AND_BACK, isWireframe ? GL_LINE : GL_FILL);
		}
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
	cameraPtr->setTranslation(cameraPtr->getTranslation() +
							  glm::vec3(0.0, 0.0, -yoffset));
}

void cursorPosCallback(GLFWwindow* windowPtr, double xpos, double ypos) {
	int width, height;
	glfwGetWindowSize(windowPtr, &width, &height);

	const auto& io = ImGui::GetIO();
	if (io.WantCaptureMouse) return;

	float normalizer = static_cast<float>((width + height) / 2);
	float dx = static_cast<float>((baseX - xpos) / normalizer);
	float dy = static_cast<float>((ypos - baseY) / normalizer);
	if (isRotating) {
		glm::vec3 dRot(-dy * M_PI, dx * M_PI, 0.0);
		cameraPtr->setRotation(baseRot + dRot);
	} else if (isPanning) {
		cameraPtr->setTranslation(baseTrans + glm::vec3(dx, dy, 0.0));
	} else if (isZooming) {
		cameraPtr->setTranslation(baseTrans + glm::vec3(0.0, 0.0, dy));
	}
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
		if (!isRotating) {
			isRotating = true;
			glfwGetCursorPos(window, &baseX, &baseY);
			baseRot = cameraPtr->getRotation();
		}
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
		isRotating = false;
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS) {
		if (!isPanning) {
			isPanning = true;
			glfwGetCursorPos(window, &baseX, &baseY);
			baseTrans = cameraPtr->getTranslation();
		}
	}
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE) {
		isPanning = false;
	}
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS) {
		if (!isZooming) {
			isZooming = true;
			glfwGetCursorPos(window, &baseX, &baseY);
			baseTrans = cameraPtr->getTranslation();
		}
	}
	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE) {
		isZooming = false;
	}
}

void windowSizeCallback(GLFWwindow* windowPtr, int width, int height) {
	cameraPtr->setAspectRatio(static_cast<float>(width) /
							  static_cast<float>(height));
}

GLuint genGPUBuffer(size_t elementSize, size_t numElements, const void* data) {
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ARRAY_BUFFER, buffer);
	glBufferData(GL_ARRAY_BUFFER, elementSize * numElements, data,
				 GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	return buffer;
}

GLuint genIndexBuffer(size_t elementSize, size_t numElements,
					  const void* data) {
	GLuint buffer;
	glGenBuffers(1, &buffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, elementSize * numElements, data,
				 GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	return buffer;
}

GLuint genVertexArray(GLuint vbo, GLuint ebo) {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBindVertexArray(0);
	return vao;
}

int main() {
	if (!glfwInit()) {
		std::cerr << "Failed to initialize GLFW" << std::endl;
		return -1;
	}

	// OpenGL context setup
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* windowPtr =
		glfwCreateWindow(800, 600, "Planet Generation", NULL, NULL);
	if (!windowPtr) {
		std::cerr << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(windowPtr);
	glfwSetFramebufferSizeCallback(windowPtr, framebuffer_size_callback);
	glfwSetKeyCallback(windowPtr, keyCallback);
	glfwSetScrollCallback(windowPtr, scroll_callback);
	glfwSetCursorPosCallback(windowPtr, cursorPosCallback);
	glfwSetMouseButtonCallback(windowPtr, mouseButtonCallback);
	glfwSetWindowSizeCallback(windowPtr, windowSizeCallback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		std::cerr << "Failed to initialize GLAD" << std::endl;
		return -1;
	}

	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	// ImGui setup
	ImGui::CreateContext();
	ImGui_ImplGlfw_InitForOpenGL(windowPtr, true);
	ImGui_ImplOpenGL3_Init("#version 460");

	// Camera setup
	int width, height;
	glfwGetWindowSize(windowPtr, &width, &height);

	cameraPtr = std::make_shared<Camera>();
	cameraPtr->setAspectRatio(static_cast<float>(width) /
							  static_cast<float>(height));
	cameraPtr->setTranslation(glm::vec3(0.0, 0.0, 3.0));
	cameraPtr->setNear(0.1f);
	cameraPtr->setFar(100.f);

	// Load shaders
	std::string shaders_folder = "../Resources/Shaders/";
	std::shared_ptr<ShaderProgram> shader =
		ShaderProgram::genBasicShaderProgram(shaders_folder + "shader.vert",
											 shaders_folder + "shader.frag");

	// Generate sphere mesh
	std::vector<glm::vec3> sphereVertices;
	std::vector<glm::uvec3> sphereIndices;
	generateSphereMesh(100, sphereVertices, sphereIndices);

	// VAO, VBO setup
	VBO = genGPUBuffer(sizeof(float) * 3, sphereVertices.size(),
					   sphereVertices.data());
	EBO = genIndexBuffer(sizeof(unsigned int) * 3, sphereIndices.size(),
						 sphereIndices.data());
	VAO = genVertexArray(VBO, EBO);

	while (!glfwWindowShouldClose(windowPtr)) {
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->use();

		shader->set("model", glm::mat4(1.0f));
		shader->set("view", cameraPtr->computeViewMatrix());
		shader->set("projection", cameraPtr->computeProjectionMatrix());

		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, sphereIndices.size() * 3, GL_UNSIGNED_INT,
					   0);
		glBindVertexArray(0);

		// ImGui UI
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGui::Begin("Performance");
		ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
		ImGui::End();
		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		glfwSwapBuffers(windowPtr);
		glfwPollEvents();
	}

	// Cleanup
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	glfwDestroyWindow(windowPtr);
	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}

// Generate a regular grid of vertices using xdir and ydir as the basis vectors
void addFace(glm::vec3 xdir, glm::vec3 ydir, int subdivisions,
			 std::vector<glm::vec3>& vertices,
			 std::vector<glm::uvec3>& indices) {
	glm::vec3 zdir = glm::cross(xdir, ydir);

	unsigned int offset = vertices.size();
	for (int i = 0; i < subdivisions; i++) {
		for (int j = 0; j < subdivisions; j++) {
			// 0 to subdivisions - 1 => -1 to 1
			float x = 2.0f * i / (float)(subdivisions - 1) - 1.0f;
			float y = 2.0f * j / (float)(subdivisions - 1) - 1.0f;
			glm::vec3 vertex = x * xdir + y * ydir + zdir;
			vertex = glm::normalize(vertex);
			vertices.push_back(vertex);
		}
	}

	for (int i = 0; i < subdivisions - 1; i++) {
		for (int j = 0; j < subdivisions - 1; j++) {
			// 0 1
			// 2 3
			int v0 = i * subdivisions + j;
			int v1 = i * subdivisions + j + 1;
			int v2 = (i + 1) * subdivisions + j;
			int v3 = (i + 1) * subdivisions + j + 1;
			indices.push_back(glm::uvec3(v0, v2, v1) + offset);
			indices.push_back(glm::uvec3(v1, v2, v3) + offset);
		}
	}
}

// Generate a sphere mesh with a given number of subdivisions
void generateSphereMesh(int subdivisions, std::vector<glm::vec3>& vertices,
						std::vector<glm::uvec3>& indices) {
	glm::vec3 xdir = glm::vec3(1.0f, 0.0f, 0.0f);
	glm::vec3 ydir = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 zdir = glm::vec3(0.0f, 0.0f, 1.0f);

	addFace(xdir, ydir, subdivisions, vertices, indices);
	addFace(-xdir, ydir, subdivisions, vertices, indices);
	addFace(-ydir, zdir, subdivisions, vertices, indices);
	addFace(ydir, zdir, subdivisions, vertices, indices);
	addFace(-xdir, zdir, subdivisions, vertices, indices);
	addFace(xdir, zdir, subdivisions, vertices, indices);
}