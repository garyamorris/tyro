#version 330 core
// Volumetric god rays — single-pass screen-space sun-shaft renderer.
//
// For each pixel we march N samples along the camera ray from the eye to
// the scene's depth-buffer hit. At each sample we project into the
// directional sun's light-space and compare against the shadow map: if the
// sample isn't shadowed, we treat that step as scattering some sun light
// toward the camera. Sum the unshadowed steps, multiply by sun colour, add
// to the scene colour. The result is the classic shaft-of-light look in
// dusty / hazy scenes.
//
// Why this works:
//   The shadow map already encodes "where the sun reaches" for the
//   directional light. Walking the camera ray IS volumetric sampling of
//   the same volume — every unshadowed sample is "lit air" between camera
//   and scene. No new data structures, no extra passes.
//
// Pipeline:
//   1. Build the world-space camera ray analytically (no inverse VP needed)
//      from the camera basis + NDC + tan(fovY/2) — same trick the picker
//      uses in src/main.cpp.
//   2. Step along it N times from t=0 to t=uMaxDist.
//   3. Per sample: project into clip space; if the sample's NDC depth is
//      past the scene depth buffer, we're behind geometry — stop early.
//   4. Per sample: project into light space; if the sample's depth is
//      <= the shadow map's recorded depth, count it as lit.
//   5. visibility = lit_count / N. Scatter contribution =
//      uSunColor * visibility * uStrength * forwardBias.
//   6. forwardBias = pow(max(dot(rayDir, -sunDir), 0), 8) — only spawn
//      shafts when the camera looks roughly toward the sun. Avoids
//      brightening every pixel uniformly.
//
// Refs:
//   Mitchell, "Volumetric Light Scattering as a Post-Process" (2007)
//   Wronski, "Volumetric Fog: Unified compute shader-based solution to
//     atmospheric scattering" (2014) — same idea, fancier pipeline

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform sampler2D uDepth;
uniform sampler2D uShadowMap;

uniform mat4  uViewProj;       // for projecting samples → NDC depth compare
uniform mat4  uLightVP;        // world → directional sun's clip space

uniform vec3  uCameraPos;
uniform vec3  uCameraFwd;      // unit, points from camera into scene
uniform vec3  uCameraRight;    // unit
uniform vec3  uCameraUp;       // unit
uniform float uTanHalfFov;     // tan(fovY * 0.5)
uniform float uAspect;

uniform vec3  uSunDir;         // direction the sun shines (same convention as Light::direction)
uniform vec3  uSunColor;       // pre-multiplied by intensity by the C++ side
uniform float uStrength;       // overall scatter scale
uniform float uMaxDist;        // how far down the ray we walk

const int   kSteps     = 32;
const float kShadowBias = 0.0008;

void main() {
    vec3 sceneColor = textureLod(uColor, vUV, 0).rgb;
    float pixelDepth = textureLod(uDepth, vUV, 0).r;

    // World-space ray from this pixel.
    float ndcX = vUV.x * 2.0 - 1.0;
    float ndcY = vUV.y * 2.0 - 1.0;
    vec3 rayDir = normalize(uCameraFwd
                            + uCameraRight * (ndcX * uTanHalfFov * uAspect)
                            + uCameraUp    * (ndcY * uTanHalfFov));

    // Forward bias: when the camera looks roughly toward the sun the shafts
    // get bright; when looking away, they fade out smoothly. Without this
    // every pixel's ray would accumulate sun light somewhere along its
    // length (the directional light reaches everywhere) and the whole
    // image would lift uniformly.
    float forward = pow(max(dot(rayDir, -normalize(uSunDir)), 0.0), 8.0);
    if (forward < 0.01) {
        FragColor = vec4(sceneColor, 1.0);
        return;
    }

    float step = uMaxDist / float(kSteps);
    // Tiny per-pixel jitter on the start offset hides banding without
    // needing a blue-noise texture.
    float jitter = fract(sin(dot(vUV, vec2(12.9898, 78.233))) * 43758.5453);

    float lit = 0.0;
    for (int i = 0; i < kSteps; ++i) {
        float t = (float(i) + jitter) * step;
        vec3  wp = uCameraPos + rayDir * t;

        // Past scene geometry? Bail.
        vec4 clip = uViewProj * vec4(wp, 1.0);
        float sampleNDCz = (clip.z / clip.w) * 0.5 + 0.5;
        if (sampleNDCz > pixelDepth) break;

        // Shadow lookup. Outside the shadow VP = treat as lit (the volume
        // is "outside the cascade", not behind a caster).
        vec4 lp = uLightVP * vec4(wp, 1.0);
        vec3 sp = lp.xyz / lp.w * 0.5 + 0.5;
        if (all(greaterThanEqual(sp.xy, vec2(0.0)))
         && all(lessThanEqual   (sp.xy, vec2(1.0)))
         && sp.z <= 1.0) {
            float closest = textureLod(uShadowMap, sp.xy, 0).r;
            if (sp.z <= closest + kShadowBias) lit += 1.0;
        } else {
            lit += 1.0;  // outside shadow frustum = unshadowed air
        }
    }

    float visibility = lit / float(kSteps);
    vec3  scatter    = uSunColor * uStrength * visibility * forward;

    FragColor = vec4(sceneColor + scatter, 1.0);
}
