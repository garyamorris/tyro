#version 330 core

#define MAX_LIGHTS 8
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1
#define LIGHT_SPOT        2

struct Light {
    int   type;
    vec3  position;
    vec3  direction;
    vec3  color;
    float intensity;
    float radius;
    float cutoffCos;
    float outerCutoffCos;
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

// Distance to nearest hex center on a unit-pitch hex grid.
float hexDist(vec2 p) {
    p = abs(p);
    return max(p.y * 0.866 + p.x * 0.5, p.x);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    vec2 uv = vUV * 8.0;
    // Hex tiling: split the plane into rectangles, find the closer of two centers.
    vec2 r = vec2(1.0, 1.732);
    vec2 h = r * 0.5;
    vec2 a = mod(uv, r) - h;
    vec2 b = mod(uv + h, r) - h;
    vec2 q = (dot(a, a) < dot(b, b)) ? a : b;
    float d = hexDist(q);

    float edge = smoothstep(0.45, 0.50, d);   // border of hex
    float core = smoothstep(0.50, 0.42, d);   // interior

    vec3 borderCol = uAlbedo * 0.20;
    vec3 fillCol   = mix(uAlbedo * 0.70, uAlbedo, core);
    vec3 albedo    = mix(fillCol, borderCol, edge);

    vec3 color = uEmissive + albedo * 0.12;
    for (int i = 0; i < uLightCount; ++i) {
        vec3 Ldir; float atten = 1.0;
        if (uLights[i].type == LIGHT_DIRECTIONAL) {
            Ldir = normalize(-uLights[i].direction);
        } else {
            vec3 dvec = uLights[i].position - vWorldPos;
            float l = length(dvec);
            Ldir = dvec / max(l, 1e-4);
            float t = clamp(1.0 - l / max(uLights[i].radius, 1e-4), 0.0, 1.0);
            atten = t * t;
            if (uLights[i].type == LIGHT_SPOT) {
                float cosTheta = dot(-Ldir, normalize(uLights[i].direction));
                float spot = clamp(
                    (cosTheta - uLights[i].outerCutoffCos)
                    / max(uLights[i].cutoffCos - uLights[i].outerCutoffCos, 1e-4),
                    0.0, 1.0);
                atten *= spot;
            }
        }
        vec3 H = normalize(Ldir + V);
        float diff = max(dot(N, Ldir), 0.0);
        float spec = pow(max(dot(N, H), 0.0), uShininess);
        color += atten * uLights[i].intensity * uLights[i].color
               * (albedo * diff + 0.4 * spec);
    }
    FragColor = vec4(color, 1.0);
}
