#version 330 core
// 2D LUT baked once at startup for the split-sum specular IBL term.
// X = NdotV, Y = roughness, output (R, G) = (scale, bias) such that
//   F * scale + bias  reconstructs the integrated environment BRDF.
// Source: "Real Shading in Unreal Engine 4" (Karis, 2014).
in  vec2 vUV;
out vec2 FragColor;

const float PI = 3.14159265358979323846;

float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 hammersley(uint i, uint N) {
    return vec2(float(i) / float(N), radicalInverse_VdC(i));
}

vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness) {
    float a = roughness * roughness;
    float phi      = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    vec3 H = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);

    vec3 up    = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tan   = normalize(cross(up, N));
    vec3 bitan = cross(N, tan);
    return normalize(tan * H.x + bitan * H.y + N * H.z);
}

float G_SchlickGGX(float NdX, float k) {
    return NdX / (NdX * (1.0 - k) + k);
}

float G_Smith_IBL(float NdV, float NdL, float roughness) {
    // For IBL the recommended k uses roughness^2 / 2 (no +1 boost).
    float a = roughness;
    float k = (a * a) / 2.0;
    return G_SchlickGGX(NdV, k) * G_SchlickGGX(NdL, k);
}

vec2 integrateBRDF(float NdotV, float roughness) {
    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;
    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H  = importanceSampleGGX(Xi, N, roughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdL = max(L.z, 0.0);
        float NdH = max(H.z, 0.0);
        float VdH = max(dot(V, H), 0.0);
        if (NdL > 0.0) {
            float G = G_Smith_IBL(max(V.z, 0.0), NdL, roughness);
            float Gvis = (G * VdH) / (NdH * max(V.z, 1e-4));
            float Fc = pow(1.0 - VdH, 5.0);
            A += (1.0 - Fc) * Gvis;
            B += Fc * Gvis;
        }
    }
    return vec2(A, B) / float(SAMPLE_COUNT);
}

void main() {
    FragColor = integrateBRDF(vUV.x, vUV.y);
}
