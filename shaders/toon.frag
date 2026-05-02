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

out vec4 FragColor;

float quantize(float v, float steps) {
    return floor(v * steps) / steps;
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    float wrap = 0.0;
    for (int i = 0; i < uLightCount; ++i) {
        vec3 Ldir; float atten = 1.0;
        if (uLights[i].type == LIGHT_DIRECTIONAL) {
            Ldir = normalize(-uLights[i].direction);
        } else {
            vec3 d = uLights[i].position - vWorldPos;
            float l = length(d);
            Ldir = d / max(l, 1e-4);
            float t = clamp(1.0 - l / max(uLights[i].radius, 1e-4), 0.0, 1.0);
            atten = t * t;
        }
        wrap += atten * uLights[i].intensity * max(dot(N, Ldir) * 0.5 + 0.5, 0.0);
    }

    // 3-band cel quantization on accumulated wrap-lit term.
    float band = quantize(clamp(wrap, 0.0, 1.0), 3.0);

    // Rim term to give it a stylised silhouette.
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);

    vec3 color = uAlbedo * (0.25 + 0.75 * band) + uEmissive + vec3(rim) * 0.4;
    FragColor = vec4(color, 1.0);
}
