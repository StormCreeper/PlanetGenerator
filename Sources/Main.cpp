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

#include "Camera.h"
#include "Mesh.h"
#include "Light.h"
#include "Material.h"

#include "editors/UIManager.h"
#include "editors/DebugEditor.h"
#include "editors/LightsEditor.h"
#include "editors/MaterialEditor.h"

#include "WorldGen.h"

#include "IO.h"

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

// Global variables
GLuint VAO, VBO, EBO;
float deltaTime = 0.0f, lastFrame = 0.0f;
std::shared_ptr<ShaderProgram> shader;

std::shared_ptr<Camera> cameraPtr;

std::vector<std::shared_ptr<AbstractLight>> lights;
Material material{glm::vec3(1.0f), 0.8f, 0.5f, glm::vec3(1.0f)};

static std::shared_ptr<UIManager> uiManager;

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
			shader = ShaderProgram::genBasicShaderProgram(
				shaders_folder + "shader.vert", shaders_folder + "shader.frag");
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

	IO::fetchTilePNG(0, 0, 0, "../Resources/Textures/Tile.png");

	// Camera setup
	int width, height;
	glfwGetWindowSize(windowPtr, &width, &height);

	cameraPtr = std::make_shared<Camera>();
	cameraPtr->setAspectRatio(static_cast<float>(width) /
							  static_cast<float>(height));
	cameraPtr->setTranslation(glm::vec3(0.0, 0.0, 3.0));
	cameraPtr->setNear(0.1f);
	cameraPtr->setFar(100.f);

	// Lights

	lights.push_back(std::make_shared<DirectionalLight>(
		glm::vec3(1.0), 4.0f, glm::normalize(glm::vec3(0.0f, -0.4f, -0.9f))));

	lights.push_back(std::make_shared<DirectionalLight>(
		glm::vec3(1.0), 4.0f,
		glm::normalize(glm::vec3(-0.94f, -0.1f, -0.33f))));

	lights.push_back(std::make_shared<DirectionalLight>(
		glm::vec3(1.0), 4.0f, glm::normalize(glm::vec3(0.4f, 0.37f, 0.84f))));

	// Load shaders
	std::string shaders_folder = "../Resources/Shaders/";
	shader = ShaderProgram::genBasicShaderProgram(
		shaders_folder + "PlanetShader.vert",
		shaders_folder + "PlanetShader.frag");

	WorldGen worldGen{};

	// Generate sphere mesh
	Mesh sphereMesh;
	// worldGen.generateSphereMesh(400, sphereMesh.positions(), sphereMesh.indices());
	std::vector<glm::vec2> positions2D;
	worldGen.generateMercatorTileMesh(4, positions2D, sphereMesh.positions(), sphereMesh.indices());

	sphereMesh.recomputePerVertexNormals();

	sphereMesh.toGPU();

	uiManager = std::make_shared<UIManager>();
	uiManager->init(windowPtr);

	uiManager->add(std::make_shared<DebugEditor>(deltaTime));
	uiManager->add(std::make_shared<LightsEditor>(lights));
	uiManager->add(std::make_shared<MaterialEditor>(material));

	while (!glfwWindowShouldClose(windowPtr)) {
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->use();

		// Constant rotation of the sphere around the y-axis
		float time = static_cast<float>(glfwGetTime());
		glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * 0.2f * 0.0f,
									  glm::vec3(0.0f, 1.0f, 0.0f));

		shader->set("model", model);
		shader->set("view", cameraPtr->computeViewMatrix());
		shader->set("projection", cameraPtr->computeProjectionMatrix());

		for (size_t i = 0; i < lights.size(); i++) {
			lights[i]->setUniforms(*shader,
								   "lights[" + std::to_string(i) + "]");
		}
		shader->set("numOfLights", (int)lights.size());

		material.setUniforms(*shader, "material");

		glm::vec3 eyePos = glm::inverse(cameraPtr->computeViewMatrix())[3];
		shader->set("eyePos", eyePos);

		sphereMesh.render();

		// ImGui UI
		uiManager->renderUIs();

		glfwSwapBuffers(windowPtr);
		glfwPollEvents();
	}

	// Cleanup
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	uiManager->shutdown();
	glfwDestroyWindow(windowPtr);
	glfwTerminate();
	return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
	glViewport(0, 0, width, height);
}