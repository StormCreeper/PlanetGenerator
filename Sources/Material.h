#pragma once

#include <glm/glm.hpp>
#include <glad/glad.h>

#include <string>

#include "ShaderProgram.h"

class Material {
   public:
	Material() : _albedo(0.5f) {}
	Material(const glm::vec3& albedo, float roughness, float metalness,
			 const glm::vec3& F0)
		: _albedo(albedo),
		  _roughness(roughness),
		  _metalness(metalness),
		  _F0(F0) {}

	void setUniforms(ShaderProgram& program, std::string name) const;

	inline glm::vec3& albedo() { return _albedo; }
	inline const glm::vec3& albedo() const { return _albedo; }

	inline float& roughness() { return _roughness; }
	inline float roughness() const { return _roughness; }

	inline float& metalness() { return _metalness; }
	inline float metalness() const { return _metalness; }

	inline glm::vec3& F0() { return _F0; }
	inline const glm::vec3& F0() const { return _F0; }

   private:
	glm::vec3 _albedo;
	float _roughness;
	glm::vec3 _F0;
	float _metalness;
};
