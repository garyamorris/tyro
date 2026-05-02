#version 330 core
// Physically-based forward shader (Cook-Torrance microfacet model).
//   D = GGX / Trowbridge-Reitz normal distribution
//   F = Schlick's Fresnel approximation
//   G = Smith's correlated geometric attenuation (Schlick-GGX form)
// Direct-light loop sums kD*(albedo/PI) + (D*G*F)/(4*NdL*NdV) over every
// light, optionally shadowing light[0] when it's the directional sun.
// Ambient is one of two paths:
//   - flat ambient (3% of albedo) when IBL is off
//   - Karis 2014 split-sum: irradiance cube * albedo for diffuse,
//     prefilter cube + 2D BRDF LUT for specular
// Output is linear HDR; tonemap + gamma happen in the post-FX final pass.
//
// Refs:
//   Cook & Torrance, "A Reflectance Model for Computer Graphics" (1982)
//   Karis, "Real Shading in Unreal Engine 4" (2014)
//   Burley, "Physically-Based Shading at Disney" (2012)

#define PI 3.14159265358979323846
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
    float cutoffCos;       // spot only: inner cone cosine
    float outerCutoffCos;  // spot only: outer cone cosine
};

in vec3 vWorldPos;
in vec3 vNormal;
in vec3 vTangent;
in vec2 vUV;

uniform vec3  uCameraPos;
uniform Light uLights[MAX_LIGHTS];
uniform int   uLightCount;

uniform vec3  uAlbedo;
uniform vec3  uEmissive;
uniform float uMetallic;
uniform float uRoughness;
uniform vec2  uUvScale;
uniform vec2  uUvOffset;

uniform sampler2D uAlbedoTex;
uniform sampler2D uNormalTex;
uniform sampler2D uMRTex;     // R: metallic, G: roughness
uniform float     uHasAlbedo;
uniform float     uHasNormal;
uniform float     uHasMR;

uniform sampler2D uShadowMap;
uniform mat4      uLightVP;
uniform float     uShadowEnabled;

// Image-based lighting (split-sum). Bound on TEX5/6/7 by Scene when iblEnabled.
uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilterMap;
uniform sampler2D   uBrdfLut;
uniform float       uIblEnabled;
uniform float       uPrefilterMaxLod;

out vec4 FragColor;

// ---- Cook-Torrance terms --------------------------------------------------
float D_GGX(float NdH, float a) {
    float a2 = a * a;
    float denom = NdH * NdH * (a2 - 1.0) + 1.0;
    return a2 / (PI * denom * denom + 1e-7);
}
float G_SchlickGGX(float NdX, float k) {
    return NdX / (NdX * (1.0 - k) + k);
}
float G_Smith(float NdV, float NdL, float roughness) {
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;            // Schlick-GGX direct-lighting k
    return G_SchlickGGX(NdV, k) * G_SchlickGGX(NdL, k);
}
vec3 F_Schlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}
// Roughness-aware Fresnel used for the IBL ambient term so that rough
// surfaces don't get over-bright Fresnel highlights at grazing angles.
vec3 F_SchlickRoughness(float cosTheta, vec3 F0, float roughness) {
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

// ---- PCF shadow (matches phong_lit's helper) ------------------------------
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

vec3 sampleNormalWS(vec2 uv) {
    vec3 N = normalize(vNormal);
    if (uHasNormal < 0.5) return N;
    vec3 nTan = texture(uNormalTex, uv).rgb * 2.0 - 1.0;
    vec3 T = normalize(vTangent - N * dot(N, vTangent)); // Gram-Schmidt
    vec3 B = cross(N, T);
    return normalize(mat3(T, B, N) * nTan);
}

void main() {
    vec2 uv = vUV * uUvScale + uUvOffset;

    vec3 albedo = uAlbedo;
    if (uHasAlbedo > 0.5) albedo *= texture(uAlbedoTex, uv).rgb;

    float metallic  = uMetallic;
    float roughness = uRoughness;
    if (uHasMR > 0.5) {
        vec2 mr = texture(uMRTex, uv).rg;
        metallic  *= mr.r;
        roughness *= mr.g;
    }
    roughness = clamp(roughness, 0.04, 1.0);

    vec3 N = sampleNormalWS(uv);
    vec3 V = normalize(uCameraPos - vWorldPos);
    float NdV = max(dot(N, V), 0.0);

    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    for (int i = 0; i < uLightCount; ++i) {
        vec3 L; vec3 radiance;
        if (uLights[i].type == LIGHT_DIRECTIONAL) {
            L = normalize(-uLights[i].direction);
            radiance = uLights[i].color * uLights[i].intensity;
        } else {
            // Point + Spot share the position-based attenuation; spot adds
            // a smoothstep cone falloff between cutoffCos and outerCutoffCos.
            vec3 d = uLights[i].position - vWorldPos;
            float dist = length(d);
            L = d / max(dist, 1e-4);
            float t = clamp(1.0 - dist / max(uLights[i].radius, 1e-4), 0.0, 1.0);
            float atten = t * t;
            if (uLights[i].type == LIGHT_SPOT) {
                float cosTheta = dot(-L, normalize(uLights[i].direction));
                float spot = clamp(
                    (cosTheta - uLights[i].outerCutoffCos)
                    / max(uLights[i].cutoffCos - uLights[i].outerCutoffCos, 1e-4),
                    0.0, 1.0);
                atten *= spot;
            }
            radiance = uLights[i].color * uLights[i].intensity * atten;
        }
        vec3 H = normalize(V + L);
        float NdL = max(dot(N, L), 0.0);
        float NdH = max(dot(N, H), 0.0);
        float HdV = max(dot(H, V), 0.0);

        float a   = roughness * roughness;
        float D   = D_GGX(NdH, a);
        float G   = G_Smith(NdV, NdL, roughness);
        vec3  F   = F_Schlick(HdV, F0);

        vec3 specular = (D * G * F) / max(4.0 * NdV * NdL, 1e-4);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 contrib = (kD * albedo / PI + specular) * radiance * NdL;
        if (i == 0 && uLights[0].type == LIGHT_DIRECTIONAL) {
            contrib *= sampleShadow(vWorldPos, N, normalize(uLights[0].direction));
        }
        Lo += contrib;
    }

    vec3 ambient;
    if (uIblEnabled > 0.5) {
        // Split-sum IBL (Karis 2014).
        //   diffuse  = irradiance(N) * albedo * kD
        //   specular = prefilter(R, roughness) * (F * brdf.x + brdf.y)
        vec3 R = reflect(-V, N);
        vec3 F  = F_SchlickRoughness(NdV, F0, roughness);
        vec3 kS = F;
        vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

        vec3 irradiance = texture(uIrradianceMap, N).rgb;
        vec3 diffuse    = irradiance * albedo;

        float lod = roughness * uPrefilterMaxLod;
        vec3 prefiltered = textureLod(uPrefilterMap, R, lod).rgb;
        vec2 brdf = texture(uBrdfLut, vec2(NdV, roughness)).rg;
        vec3 specular = prefiltered * (F * brdf.x + brdf.y);

        ambient = kD * diffuse + specular;
    } else {
        ambient = vec3(0.03) * albedo;
    }
    vec3 color = ambient + Lo + uEmissive;

    // Output linear HDR; the post-FX chain's final pass owns tonemap + sRGB.
    FragColor = vec4(color, 1.0);
}
