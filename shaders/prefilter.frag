#version 330 core
// Prefiltered radiance map (split-sum approximation, Karis 2014).
// We GGX-importance-sample the env cube into the destination face/mip level,
// where each mip corresponds to a fixed roughness in [0,1]. The PBR shader
// later samples this with textureLod(prefilterMap, R, roughness * MAX_MIP).
in  vec3 vDir;
out vec4 FragColor;

uniform samplerCube uEnv;
uniform float       uRoughness;
uniform float       uEnvFaceSize;  // for mip-bias to reduce sample variance

const float PI = 3.14159265358979323846;

float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
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

float D_GGX(float NdH, float a) {
    float a2 = a * a;
    float d  = NdH * NdH * (a2 - 1.0) + 1.0;
    return a2 / (PI * d * d + 1e-7);
}

void main() {
    vec3 N = normalize(vDir);
    vec3 R = N;     // assume V == N for the prefilter (standard Karis assumption)
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3  prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H  = importanceSampleGGX(Xi, N, uRoughness);
        vec3 L  = normalize(2.0 * dot(V, H) * H - V);

        float NdL = max(dot(N, L), 0.0);
        if (NdL > 0.0) {
            // Mip-bias to reduce hot-pixel variance for low-roughness mips
            // (Krivanek/Colbert "filtered importance sampling").
            float a   = uRoughness * uRoughness;
            float NdH = max(dot(N, H), 0.0);
            float HdV = max(dot(H, V), 0.0);
            float D   = D_GGX(NdH, a);
            float pdf = D * NdH / (4.0 * HdV) + 1e-4;

            float saTexel  = 4.0 * PI / (6.0 * uEnvFaceSize * uEnvFaceSize);
            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 1e-4);
            float mipLevel = uRoughness == 0.0
                ? 0.0
                : 0.5 * log2(saSample / saTexel);

            prefilteredColor += textureLod(uEnv, L, mipLevel).rgb * NdL;
            totalWeight      += NdL;
        }
    }
    prefilteredColor /= totalWeight;
    FragColor = vec4(prefilteredColor, 1.0);
}
