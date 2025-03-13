#include "Mesh.h"

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

GLuint genVertexArray(GLuint pos_vbo, GLuint norm_vbo, GLuint ebo) {
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, pos_vbo);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, norm_vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBindVertexArray(0);
	return vao;
}

void Mesh::toGPU() {
	_posVbo =
		genGPUBuffer(sizeof(float) * 3, _positions.size(), _positions.data());

	_normVbo =
		genGPUBuffer(sizeof(float) * 3, _normals.size(), _normals.data());

	_ebo = genIndexBuffer(sizeof(unsigned int) * 3, _indices.size(),
						  _indices.data());
	_vao = genVertexArray(_posVbo, _normVbo, _ebo);
}

void Mesh::render() {
	glBindVertexArray(_vao);
	glDrawElements(GL_TRIANGLES, _indices.size() * 3, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void Mesh::recomputePerVertexNormals() {
	_normals.clear();
	_normals.resize(_positions.size(), glm::vec3(0.0, 0.0, 0.0));
#pragma omp parallel
	for (auto& t : _indices) {
		glm::vec3 e0(_positions[t[1]] - _positions[t[0]]);
		glm::vec3 e1(_positions[t[2]] - _positions[t[0]]);
		glm::vec3 n = glm::normalize(glm::cross(e0, e1));
		for (size_t i = 0; i < 3; i++)
			_normals[t[glm::vec3::length_type(i)]] += n;
	}
	for (auto& n : _normals) n = glm::normalize(n);
}