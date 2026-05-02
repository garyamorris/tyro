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

in vec3 gWorldPos;
in vec3 gFaceNormal;

uniform vec3  uCameraPos;
uniform Light uLights[MAX_LIGHTS];
uniform int   uLightCount;
uniform vec3  uAlbedo;
uniform vec3  uEmissive;
uniform float uShininess;

out vec4 FragColor;

void main() {
    vec3 N = normalize(gFaceNormal);
    vec3 V = normalize(uCameraPos - gWorldPos);

    vec3 color = uEmissive + uAlbedo * 0.12;
    for (int i = 0; i < uLightCount; ++i) {
        vec3 Ldir; float atten = 1.0;
        if (uLights[i].type == LIGHT_DIRECTIONAL) {
            Ldir = normalize(-uLights[i].direction);
        } else {
            vec3 d = uLights[i].position - gWorldPos;
            float l = length(d);
            Ldir = d / max(l, 1e-4);
            float t = clamp(1.0 - l / max(uLights[i].radius, 1e-4), 0.0, 1.0);
            atten = t * t;
        }
        vec3 H = normalize(Ldir + V);
        float diff = max(dot(N, Ldir), 0.0);
        float spec = pow(max(dot(N, H), 0.0), uShininess);
        color += atten * uLights[i].intensity * uLights[i].color
               * (uAlbedo * diff + 0.4 * spec);
    }
    FragColor = vec4(color, 1.0);
}
