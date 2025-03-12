#version 460 core
out vec4 FragColor;

const float PI = 3.14159265358979323846;

float sqr(float x) {
    return x * x;
}

in vec3 fNormal;
in vec3 fPos;

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

void main() {
    vec3 radiance = vec3(0);

    Ray ray;
    ray.origin = eyePos;
    ray.direction = normalize(fPos - eyePos);

    for(int i=0; i<numOfLights; i++) {
        radiance += evaluateRadiance(material, lights[i], fNormal, fPos, ray);
    }
    FragColor = vec4(radiance, 1.0);
}
