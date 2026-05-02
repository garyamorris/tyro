#version 330 core

#define MAX_LIGHTS 8
#define LIGHT_DIRECTIONAL 0
#define LIGHT_POINT       1

struct Light {
    int   type;
    vec3  position;
    vec3  direction;  // for directional, points in light-shine direction
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

uniform sampler2D uAlbedoTex;
uniform float     uHasAlbedo;
uniform vec2      uUvScale;
uniform vec2      uUvOffset;

uniform sampler2D uShadowMap;
uniform mat4      uLightVP;
uniform float     uShadowEnabled;

out vec4 FragColor;

float sampleShadow(vec3 worldPos, vec3 N, vec3 lightDir) {
    if (uShadowEnabled < 0.5) return 1.0;
    vec4 lp = uLightVP * vec4(worldPos, 1.0);
    vec3 proj = lp.xyz / lp.w;
    proj = proj * 0.5 + 0.5;
    if (proj.z > 1.0 || proj.x < 0.0 || proj.x > 1.0 ||
                       proj.y < 0.0 || proj.y > 1.0) return 1.0;
    float bias = max(0.0035 * (1.0 - dot(N, -lightDir)), 0.0008);
    vec2 ts = 1.0 / vec2(textureSize(uShadowMap, 0));
    float lit = 0.0;
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float closest = texture(uShadowMap, proj.xy + vec2(x, y) * ts).r;
            lit += (proj.z - bias) > closest ? 0.0 : 1.0;
        }
    }
    return lit / 9.0;
}

vec3 shade(Light L, vec3 N, vec3 V, vec3 worldPos, vec3 albedo) {
    vec3 Ldir; float atten = 1.0;
    if (L.type == LIGHT_DIRECTIONAL) {
        Ldir = normalize(-L.direction);
    } else {
        vec3 d = L.position - worldPos;
        float l = length(d);
        Ldir = d / max(l, 1e-4);
        float t = clamp(1.0 - l / max(L.radius, 1e-4), 0.0, 1.0);
        atten = t * t;
    }
    vec3 H = normalize(Ldir + V);
    float diffuse  = max(dot(N, Ldir), 0.0);
    float specular = pow(max(dot(N, H), 0.0), uShininess);
    return atten * L.intensity * L.color * (albedo * diffuse + 0.4 * specular);
}

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    vec3 albedo = uAlbedo;
    if (uHasAlbedo > 0.5) {
        vec2 uv = vUV * uUvScale + uUvOffset;
        albedo *= texture(uAlbedoTex, uv).rgb;
    }

    vec3 color = uEmissive + albedo * 0.10;
    for (int i = 0; i < uLightCount; ++i) {
        vec3 contrib = shade(uLights[i], N, V, vWorldPos, albedo);
        // Apply shadow only to lights[0] when it's the directional sun.
        if (i == 0 && uLights[0].type == LIGHT_DIRECTIONAL) {
            contrib *= sampleShadow(vWorldPos, N, normalize(uLights[0].direction));
        }
        color += contrib;
    }
    FragColor = vec4(color, 1.0);
}
