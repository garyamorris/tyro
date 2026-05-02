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
in vec2 vUV;

uniform vec3  uCameraPos;
uniform Light uLights[MAX_LIGHTS];
uniform int   uLightCount;
uniform vec3  uAlbedo;
uniform vec3  uEmissive;
uniform float uShininess;

out vec4 FragColor;

float hash(vec2 p) { return fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453); }

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    vec2 uv = vUV * 6.0;
    float row     = floor(uv.y);
    float rowOff  = mod(row, 2.0) * 0.5;
    float xInRow  = fract(uv.x + rowOff);
    float yInRow  = fract(uv.y);

    float mortar = 0.06;
    bool isMortar = (xInRow < mortar) || (xInRow > 1.0 - mortar)
                 || (yInRow < mortar) || (yInRow > 1.0 - mortar);

    vec2 brickId = vec2(floor(uv.x + rowOff), row);
    float jitter = hash(brickId);
    vec3 brickCol = uAlbedo * (0.85 + 0.30 * jitter);
    vec3 mortarCol = vec3(0.32);

    vec3 albedo = isMortar ? mortarCol : brickCol;

    vec3 color = uEmissive + albedo * 0.12;
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
        vec3 H = normalize(Ldir + V);
        float diff = max(dot(N, Ldir), 0.0);
        float spec = pow(max(dot(N, H), 0.0), uShininess);
        color += atten * uLights[i].intensity * uLights[i].color
               * (albedo * diff + 0.10 * spec);
    }
    FragColor = vec4(color, 1.0);
}
