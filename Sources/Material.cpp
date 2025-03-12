#include "Material.h"

void Material::setUniforms(ShaderProgram& program, std::string name) const {
	program.set(name + ".albedo", _albedo);
	program.set(name + ".roughness", _roughness);
	program.set(name + ".metalness", _metalness);
	program.set(name + ".F0", _F0);
}