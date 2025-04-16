#pragma once

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glad/glad.h>

#include <vector>

class Mesh {
   public:
	std::vector<glm::vec3> &positions() { return _positions; }
	std::vector<glm::vec3> &normals() { return _normals; }
	std::vector<glm::vec2> &texCoords() { return _texCoords; }
	std::vector<glm::uvec3> &indices() { return _indices; }

	void toGPU();
	void render();

	void recomputePerVertexNormals();

   private:
	std::vector<glm::vec3> _positions;
	std::vector<glm::vec3> _normals;
	std::vector<glm::vec2> _texCoords;
	std::vector<glm::uvec3> _indices;

	GLuint _vao;
	GLuint _posVbo;
	GLuint _normVbo;
	GLuint _texVbo;
	GLuint _ebo;
};