#version 330 core
// FXAA — Fast Approximate Anti-Aliasing (Lite variant of NVIDIA FXAA 3.11).
// One-pass post-process that softens visible-luminance edges by blending
// perpendicular to the local gradient. Cheap (~14 texture samples), no
// extra geometry pass, no resolve buffer.
//
// Pipeline:
//   1. Sample the centre + 4 corner neighbours
//   2. Compute min/max luminance — if the contrast is below threshold,
//      this pixel is not on an edge, return it unchanged
//   3. Estimate the edge direction from the corner luma gradient
//   4. Blend two pairs of taps along that direction, pick whichever stays
//      inside the local luma min/max (to avoid over-blurring corners)
//
// Refs:
//   Lottes, "FXAA 3.11" white paper (NVIDIA, 2011)
//   Implementation lifted from the FXAA "Console" path — simpler than the
//   "Quality" path with subpixel detection but enough for a teaching demo.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform vec2      uTexelSize;

const float kEdgeThreshold    = 0.0833;  // no AA below this luma delta
const float kEdgeThresholdMin = 0.0625;  // hard floor for very dark pixels
const float kSpanMax          = 8.0;     // clamp the search direction

float luma(vec3 c) { return dot(c, vec3(0.299, 0.587, 0.114)); }

void main() {
    vec2 t = uTexelSize;

    vec3 cM  = texture(uColor, vUV).rgb;
    vec3 cNW = texture(uColor, vUV + vec2(-t.x, -t.y)).rgb;
    vec3 cNE = texture(uColor, vUV + vec2( t.x, -t.y)).rgb;
    vec3 cSW = texture(uColor, vUV + vec2(-t.x,  t.y)).rgb;
    vec3 cSE = texture(uColor, vUV + vec2( t.x,  t.y)).rgb;

    float lM  = luma(cM);
    float lNW = luma(cNW);
    float lNE = luma(cNE);
    float lSW = luma(cSW);
    float lSE = luma(cSE);

    float lumaMin = min(lM, min(min(lNW, lNE), min(lSW, lSE)));
    float lumaMax = max(lM, max(max(lNW, lNE), max(lSW, lSE)));

    // Below threshold = no visible edge here, skip work.
    if (lumaMax - lumaMin < max(kEdgeThresholdMin, lumaMax * kEdgeThreshold)) {
        FragColor = vec4(cM, 1.0);
        return;
    }

    // Gradient direction from corner luma — points along the edge.
    vec2 dir;
    dir.x = -((lNW + lNE) - (lSW + lSE));
    dir.y =  ((lNW + lSW) - (lNE + lSE));

    // Normalize the direction so its smallest component = 1 texel, then
    // clamp the span. The reduce term prevents division-by-zero when an
    // edge is exactly axis-aligned.
    float dirReduce  = max((lNW + lNE + lSW + lSE) * 0.25 * 0.125, 1.0 / 128.0);
    float rcpDirMin  = 1.0 / (min(abs(dir.x), abs(dir.y)) + dirReduce);
    dir = clamp(dir * rcpDirMin, vec2(-kSpanMax), vec2(kSpanMax)) * t;

    // Two-tap and four-tap averages along the edge direction. The four-tap
    // gives smoother blends but can over-blur near corners; we prefer it
    // only when its luma stays inside the local min/max.
    vec3 a = 0.5 * (
        texture(uColor, vUV + dir * (1.0/3.0 - 0.5)).rgb +
        texture(uColor, vUV + dir * (2.0/3.0 - 0.5)).rgb);
    vec3 b = a * 0.5 + 0.25 * (
        texture(uColor, vUV + dir * -0.5).rgb +
        texture(uColor, vUV + dir *  0.5).rgb);

    float lb = luma(b);
    FragColor = vec4((lb < lumaMin || lb > lumaMax) ? a : b, 1.0);
}
