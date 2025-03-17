#version 460 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec3 fNormal;
out vec3 fPos;
out vec3 fPos_model;

void main() {
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    mat4 normalMatrix = transpose(inverse(model));
    fNormal = normalize(vec3(normalMatrix * vec4(aNorm, 0.0)));
    fPos = vec3(model * vec4(aPos, 1.0));
    fPos_model = aPos;
}
