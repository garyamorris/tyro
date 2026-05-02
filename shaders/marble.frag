#version 330 core

#define MAX_LIGHTS 8
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1

struct Light {
    int   type;
    vec3  position;
    vec3  direction;
    vec3  color;
    float intensity;
    float radius;
};

in vec3 vWorldPos;
in vec3 vNormal;

uniform vec3  uCameraPos;
uniform Light uLights[MAX_LIGHTS];
uniform int   uLightCount;
uniform vec3  uAlbedo;
uniform vec3  uEmissive;
uniform float uShininess;

out vec4 FragColor;

float hash(vec3 p) {
    return fract(sin(dot(p, vec3(12.9898, 78.233, 37.719))) * 43758.5453);
}
float vnoise(vec3 p) {
    vec3 i = floor(p), f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    return mix(
        mix(mix(hash(i + vec3(0,0,0)), hash(i + vec3(1,0,0)), f.x),
            mix(hash(i + vec3(0,1,0)), hash(i + vec3(1,1,0)), f.x), f.y),
        mix(mix(hash(i + vec3(0,0,1)), hash(i + vec3(1,0,1)), f.x),
            mix(hash(i + vec3(0,1,1)), hash(i + vec3(1,1,1)), f.x), f.y), f.z);
}
float fbm(vec3 p) {
    float v = 0.0, a = 0.5;
    for (int i = 0; i < 5; ++i) { v += a * vnoise(p); p *= 2.0; a *= 0.5; }
    return v;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    // Turbulence-modulated banding gives the marble veins.
    float t = sin(vWorldPos.x * 1.6 + fbm(vWorldPos * 2.5) * 6.0);
    t = 0.5 + 0.5 * t;
    t = pow(t, 6.0);
    vec3 base = mix(uAlbedo * 0.85, vec3(1.0, 0.97, 0.94), t);
    vec3 vein = vec3(0.10, 0.18, 0.30) * pow(t, 30.0) * 1.5;
    vec3 albedo = base - vein;

    vec3 color = uEmissive + albedo * 0.10;
    for (int i = 0; i < uLightCount; ++i) {
        vec3 Ldir; float atten = 1.0;
        if (uLights[i].type == LIGHT_DIRECTIONAL) {
            Ldir = normalize(-uLights[i].direction);
        } else {
            vec3 d = uLights[i].position - vWorldPos;
            float l = length(d);
            Ldir = d / max(l, 1e-4);
            float r = clamp(1.0 - l / max(uLights[i].radius, 1e-4), 0.0, 1.0);
            atten = r * r;
        }
        vec3 H = normalize(Ldir + V);
        float diff = max(dot(N, Ldir), 0.0);
        float spec = pow(max(dot(N, H), 0.0), uShininess);
        color += atten * uLights[i].intensity * uLights[i].color
               * (albedo * diff + 0.6 * spec);
    }
    FragColor = vec4(color, 1.0);
}
