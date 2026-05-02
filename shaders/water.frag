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
uniform vec3  uAlbedo;     // shallow tint (used as fallback)
uniform vec3  uEmissive;
uniform float uTime;

out vec4 FragColor;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    vec3 deep    = vec3(0.02, 0.10, 0.18);
    vec3 shallow = mix(vec3(0.10, 0.40, 0.55), uAlbedo, 0.0); // ignore uAlbedo for now
    float fres = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    vec3 base = mix(deep, shallow, 1.0 - fres);

    // Sun specular highlight (lights[0] expected to be directional).
    vec3 sunDir = (uLightCount > 0 && uLights[0].type == LIGHT_DIRECTIONAL)
                  ? normalize(-uLights[0].direction) : normalize(vec3(0.4, 1.0, 0.3));
    vec3 H = normalize(sunDir + V);
    float spec = pow(max(dot(N, H), 0.0), 120.0);
    base += vec3(1.0, 0.95, 0.8) * spec * 1.5;

    // Subtle scrolling foam — fakes high-frequency caustics on top of the
    // low-frequency wave displacement.
    vec2 uv = vUV * 5.0 + vec2(uTime * 0.05, uTime * 0.03);
    float foam = pow(0.5 + 0.5 * sin(uv.x * 7.0 + sin(uv.y * 5.0)), 8.0);
    base += vec3(foam) * 0.18;

    base += uEmissive;
    FragColor = vec4(base, 0.95);
}
