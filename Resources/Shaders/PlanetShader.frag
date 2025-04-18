#version 460 core
out vec4 FragColor;

const float PI = 3.14159265358979323846;

float sqr(float x) {
    return x * x;
}

uniform sampler2D tex_diffuse;

in vec3 fNormal;
in vec3 fPos;
in vec3 fPos_model;
in vec2 fTexCoord;

#define MAX_LIGHTS 10

struct LightSource {
    int type; // 0: directional, 1: point
    vec3 direction; // Directional light: direction, Point light: position
    vec3 color;
    float intensity;

    // For point lights
    float ac;
    float al;
    float aq;
};

uniform LightSource lights[MAX_LIGHTS];
uniform int numOfLights;

struct Material {
    vec3 albedo;
    float roughness;
    vec3 F0;
    float metalness;
};

uniform Material material;

struct Ray {
    vec3 origin;
    vec3 direction;
    vec3 inv_direction;
};

uniform vec3 eyePos;

// Trowbridge-Reitz Normal Distribution
float NormalDistributionGGX(vec3 n, vec3 wh, float alpha) {
    float nom = sqr(alpha);
    float denom = PI * sqr(1 + (sqr(alpha) - 1.0) * sqr(dot(n, wh)));

    return nom / denom;
}

// Schlick approx for Fresnel term
vec3 Fresnel(vec3 wi, vec3 wh, vec3 F0) {
    return F0 + (1-F0) * pow(1 - max(0, dot(wi, wh)), 5);
}

// Schlick approximation for geometric term
float G_Schlick(vec3 n, vec3 w, float alpha) {
    float k = alpha * sqrt(2.0 / PI);
    float dotNW = dot(n, w);
    return dotNW / (dotNW * (1-k) + k);
}

float G_GGX(vec3 n, vec3 wi, vec3 wo, float alpha) {
    return G_Schlick(n, wi, alpha) * G_Schlick(n, wo, alpha);
}

// F = Fs (specular) + Fd (diffuse)

// Fs(wi, wo) = D*F*G / (4*dot(wi, n)*dot(wo, n))

vec3 F_Specular(vec3 n, vec3 wi, vec3 wo, vec3 wh, vec3 F0, float alpha) {
    float D = NormalDistributionGGX(n, wh, alpha);
    vec3 F = Fresnel(wi, wh, F0);
    float G = G_GGX(n, wi, wo, alpha);

    return D * F * G / (4*dot(wi, n)*dot(wo, n));
}

vec3 evaluateRadianceDirectional(Material mat, LightSource source, vec3 n, vec3 pos, Ray ray) {
    vec3 fd = mat.albedo * source.color * source.intensity / PI;

    vec3 wi = -normalize(source.direction); // Light dir
    vec3 wo = normalize(ray.origin - pos);       // Eye dir

    vec3 wh = normalize(wi + wo);

    float alpha = sqr(mat.roughness);

    vec3 fs = F_Specular(n, wi, wo, wh, mat.F0, alpha) * source.color * source.intensity;

    return (fd * (1.0 - mat.metalness) + mat.metalness * fs) * max(dot(wi, n), 0.0);
}

vec3 evaluateRadiancePointLight(Material mat, LightSource source, vec3 normal, vec3 pos, Ray ray) {
    float dist = length(source.direction - pos);
    float attenuation = 1.0 / (source.ac + dist * source.al + dist * dist * source.aq);
    vec3 lightDir = -normalize(source.direction - pos);

    LightSource new_source = LightSource(1, lightDir, source.color, source.intensity * attenuation, 0.0, 0.0, 0.0);

    return evaluateRadianceDirectional(mat, new_source, normal, pos, ray);
}

vec3 evaluateRadiance(Material mat, LightSource source, vec3 normal, vec3 pos, Ray ray) {
    if (source.type == 0) {
        return evaluateRadianceDirectional(mat, source, normal, pos, ray);
    } else {
        return evaluateRadiancePointLight(mat, source, normal, pos, ray);
    }
}


// Colors the surface of a procedural sphere planet
void main() {
    vec3 radiance = vec3(0);

    Ray ray;
    ray.origin = eyePos;
    ray.direction = normalize(fPos - eyePos);

    vec3 normal = normalize(fNormal);
    vec3 worldUp = normalize(fPos);

    float steepness = 1 - pow(abs(dot(normal, worldUp)), 3);

    /*Material mat = material;

    // 1 -> green, 0 -> orange
    mat.albedo = mix(vec3(0.1, 0.7, 0.0), vec3(0.8, 0.2, 0.0), steepness);

    float height = length(fPos);
    float radius = 1.0;
    float oceanHeight = 0.02;
    float beachHeight = 0.02;
    float snowHeight = 1.3;
    
    float sinOff = sin(dot(fPos_model, vec3(1, 1, 1)) * 10.) * 0.03;
    sinOff += sin(dot(fPos_model, vec3(1, 0.8, -1.1)) * 15.) * 0.02;
    sinOff += sin(dot(fPos_model, vec3(-1, 0.8, 1.1)) * 150.) * 0.01;
    sinOff += sin(dot(fPos_model, vec3(1, 0.8, -1.1)) * 130.) * 0.01;
    bool snow = abs(dot(worldUp, vec3(0, 1, 0))) > (0.9 + sinOff) || height > snowHeight;

    if (height < radius + oceanHeight) {
        mat.albedo = vec3(0.0, 0.0, 0.8);
        mat.roughness = 0.3;
        mat.metalness = 0.9;
    } else if (height < radius + oceanHeight + beachHeight) {
        mat.albedo = mix(vec3(1.0, 1.0, 0.2), mat.albedo, (height - radius - oceanHeight) / beachHeight);
    }

    if(snow) {
        mat.albedo = vec3(1.0);
        mat.roughness = 0.7;
        mat.metalness = 0.3;
    }

    for(int i=0; i<numOfLights; i++) {
        radiance += evaluateRadiance(mat, lights[i], normal, fPos, ray);
    }*/

    vec3 albedo = texture(tex_diffuse, fTexCoord).xyz;

    FragColor = vec4(albedo, 1.0);
}
