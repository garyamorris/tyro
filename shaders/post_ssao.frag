#version 330 core

// A cheap depth-only screen-space AO. We sample the depth buffer in a small
// disc around the current pixel and compare to current depth. If neighbours
// are noticeably closer to the camera, the current pixel is occluded.
//
// This is not "real" SSAO (which needs view-space positions and a hemispherical
// kernel) but it's effective and self-contained.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform sampler2D uDepth;
uniform vec2      uTexelSize;
uniform float     uNear;
uniform float     uFar;
uniform float     uRadius;     // in NDC space
uniform float     uIntensity;

float linearizeDepth(float d) {
    float z = d * 2.0 - 1.0;
    return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
}

void main() {
    float centerD = linearizeDepth(texture(uDepth, vUV).r);

    const int kSamples = 12;
    // Pre-baked sample directions (golden-angle disc).
    vec2 dirs[12] = vec2[](
        vec2( 1.0,  0.0), vec2( 0.5,  0.866),
        vec2(-0.5,  0.866), vec2(-1.0,  0.0),
        vec2(-0.5, -0.866), vec2( 0.5, -0.866),
        vec2( 0.707,  0.707), vec2(-0.707,  0.707),
        vec2(-0.707, -0.707), vec2( 0.707, -0.707),
        vec2( 0.0,  1.0), vec2( 0.0, -1.0)
    );

    float occ = 0.0;
    for (int i = 0; i < kSamples; ++i) {
        vec2 off = dirs[i] * uRadius * uTexelSize * 8.0;
        float d  = linearizeDepth(texture(uDepth, vUV + off).r);
        // If sample is closer than current by a small margin, count as occluder.
        float diff = centerD - d;
        occ += smoothstep(0.0, 0.5, diff) * (1.0 - smoothstep(0.5, 2.0, diff));
    }
    float ao = 1.0 - clamp(occ / float(kSamples) * uIntensity, 0.0, 1.0);

    FragColor = vec4(texture(uColor, vUV).rgb * ao, 1.0);
}
