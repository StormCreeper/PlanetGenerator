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

#include <FastNoise/FastNoise.h>

#include "Camera.h"
#include "Mesh.h"
#include "Light.h"
#include "Material.h"

// Function prototypes
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void generateSphereMesh(int subdivisions, std::vector<glm::vec3>& vertices,
						std::vector<glm::uvec3>& indices);

// Global variables
GLuint VAO, VBO, EBO;
float deltaTime = 0.0f, lastFrame = 0.0f;
std::shared_ptr<ShaderProgram> shader;

std::shared_ptr<Camera> cameraPtr;

std::vector<std::shared_ptr<AbstractLight>> lights;
Material material{glm::vec3(1.0f), 0.8f, 0.5f, glm::vec3(1.0f)};

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

void renderMaterialUI(Material& mat, int id) {
	ImGui::ColorEdit3(("Albedo##In" + std::to_string(id)).c_str(),
					  &mat.albedo().x);

	ImGui::SliderFloat(("Roughness##" + std::to_string(id)).c_str(),
					   &mat.roughness(), 0.0f, 1.0f);

	ImGui::SliderFloat(("Metalness##" + std::to_string(id)).c_str(),
					   &mat.metalness(), 0.0f, 1.0f);

	ImGui::ColorEdit3(("F0##" + std::to_string(id)).c_str(), &mat.F0().x);
}

void renderUI() {
	ImGui::NewFrame();
	ImGui::Begin("Performance", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
	ImGui::Text("FPS: %.1f", 1.0f / deltaTime);
	ImGui::End();

	ImGui::Begin("Lights", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	const char* items[] = {"Directional", "Point"};

	for (int i = 0; i < lights.size(); i++) {
		AbstractLight& light = *lights[i];
		if (ImGui::CollapsingHeader(
				std::string("Light " + std::to_string(i)).c_str())) {
			ImGui::Indent(10.0f);
			const char* comboLabel = items[light.getType()];

			if (ImGui::BeginCombo(
					("Type##" + std::to_string(i)).c_str(),
					comboLabel)) {	// Combo box for type selection
				for (int n = 0; n < IM_ARRAYSIZE(items); n++) {
					const bool is_selected = (comboLabel == items[n]);
					if (ImGui::Selectable(items[n], is_selected)) {
						if (n != light.getType()) {
							std::shared_ptr<AbstractLight> new_light;
							switch (n) {
								case 0:
									new_light =
										std::make_shared<DirectionalLight>(
											light.color(),
											light.baseIntensity(),
											glm::vec3(0.0, -1.0, 0.0));
									break;
								case 1:
									new_light = std::make_shared<PointLight>(
										light.color(), light.baseIntensity(),
										glm::vec3(0.0), 1.0, 0.0, 0.0);
							}
							lights[i] = new_light;
							light = *new_light;
						};
					}
					if (is_selected) {
						ImGui::SetItemDefaultFocus();  // You may set the
													   // initial focus when
													   // opening the combo
													   // (scrolling + for
													   // keyboard navigation
													   // support)
					}
				}
				ImGui::EndCombo();
			}
			// Common properties: color and intensity
			glm::vec3 color = light.color();
			float intensity = light.baseIntensity();
			ImGui::ColorEdit3(("Color##" + std::to_string(i)).c_str(),
							  &color.x);
			ImGui::SliderFloat(("Intensity##" + std::to_string(i)).c_str(),
							   &intensity, 0.0f, 10.0f);
			light.color() = color;
			light.baseIntensity() = intensity;

			if (light.getType() == 0) {	 // Directional light
				std::shared_ptr<DirectionalLight> dirLight =
					std::dynamic_pointer_cast<DirectionalLight>(lights[i]);
				glm::vec3 dir = dirLight->getDirection();
				ImGui::SliderFloat3(("Direction##" + std::to_string(i)).c_str(),
									&dir.x, -1.0f, 1.0f);
				dirLight->setDirection(dir);
			} else if (light.getType() == 1) {	// Point light
				std::shared_ptr<PointLight> pointLight =
					std::dynamic_pointer_cast<PointLight>(lights[i]);
				glm::vec3 pos = pointLight->getTranslation();
				ImGui::SliderFloat3(("Position##" + std::to_string(i)).c_str(),
									&pos.x, -10.0f, 10.0f);
				pointLight->setTranslation(pos);
				ImGui::SliderFloat(
					("Attenuation constant##" + std::to_string(i)).c_str(),
					&pointLight->attenuationConstant(), 0.0f, 1.0f);
				ImGui::SliderFloat(
					("Attenuation linear##" + std::to_string(i)).c_str(),
					&pointLight->attenuationLinear(), 0.0f, 1.0f);
				ImGui::SliderFloat(
					("Attenuation quadratic##" + std::to_string(i)).c_str(),
					&pointLight->attenuationQuadratic(), 0.0f, 1.0f);
			}
			ImGui::Indent(-10.0f);
		}
	}
	if (lights.size() < 10) {
		if (ImGui::Button("Add light")) {
			lights.push_back(std::make_shared<PointLight>(
				glm::vec3(1.0f), 1.0f, glm::vec3(0.0f), 1.0f, 0.0f, 0.0f));
		}
		ImGui::SameLine();
	}
	if (lights.size() > 0 && ImGui::Button("Remove light")) {
		lights.erase(lights.begin() + lights.size() - 1);
	}

	ImGui::End();

	ImGui::Begin("Materials", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

	renderMaterialUI(material, 0);

	ImGui::End();

	ImGui::Render();
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
		shaders_folder + "shader.vert", shaders_folder + "shader.frag");

	// Generate sphere mesh
	Mesh sphereMesh;
	generateSphereMesh(400, sphereMesh.positions(), sphereMesh.indices());

	sphereMesh.recomputePerVertexNormals();

	sphereMesh.toGPU();

	while (!glfwWindowShouldClose(windowPtr)) {
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		shader->use();

		// Constant rotation of the sphere around the y-axis
		float time = static_cast<float>(glfwGetTime());
		glm::mat4 model = glm::rotate(glm::mat4(1.0f), time * 0.2f,
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
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		renderUI();
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

float getHeight(const glm::vec3& normPos,
				FastNoise::SmartNode<FastNoise::FractalFBm>& fn);

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

	auto fnSimplex = FastNoise::New<FastNoise::Simplex>();
	auto fnFractal = FastNoise::New<FastNoise::FractalFBm>();
	fnFractal->SetSource(fnSimplex);
	fnFractal->SetOctaveCount(10);

#pragma omp parallel for
	for (glm::vec3& vertex : vertices) {
		vertex = glm::normalize(vertex);
		vertex *= getHeight(vertex, fnFractal);
	}
}

float getHeight(const glm::vec3& normPos,
				FastNoise::SmartNode<FastNoise::FractalFBm>& fn) {
	float height = fn->GenSingle3D(normPos.x, normPos.y, normPos.z, 0);

	height = 0.5f * height + 0.5f;
	float ocean = 1.0f - height;
	height = glm::pow(height, 4);
	ocean = 1.0f - glm::pow(ocean, 3);

	// Ocean == 1 -> height = 0, ocean = 0 -> height = height
	height = glm::smoothstep(0.0f, 1.0f, ocean) * height;

	return 1 + height * 0.5f;
}