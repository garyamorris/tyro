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

uniform sampler2D uShadowMap;
uniform mat4      uLightVP;
uniform float     uShadowEnabled;

out vec4 FragColor;

float checker(vec3 p, float scale) {
    vec3 q = floor(p * scale);
    return mod(q.x + q.y + q.z, 2.0);
}

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

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(uCameraPos - vWorldPos);

    float c = checker(vWorldPos, 1.0);
    vec3 albedo = mix(uAlbedo * 0.25, uAlbedo, c);

    vec3 color = albedo * 0.10 + uEmissive;
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
        vec3 contrib = atten * uLights[i].intensity * uLights[i].color
                     * (albedo * diff + 0.3 * spec);
        if (i == 0 && uLights[0].type == LIGHT_DIRECTIONAL) {
            contrib *= sampleShadow(vWorldPos, N, normalize(uLights[0].direction));
        }
        color += contrib;
    }
    FragColor = vec4(color, 1.0);
}
