#version 330 core

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform sampler2D uDepth;
uniform float     uNear;
uniform float     uFar;
uniform vec3      uFogColor;
uniform float     uFogStart;
uniform float     uFogEnd;

float linearizeDepth(float d) {
    float z = d * 2.0 - 1.0;
    return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear));
}

void main() {
    vec3  scene = texture(uColor, vUV).rgb;
    float depth = texture(uDepth, vUV).r;
    float linZ  = linearizeDepth(depth);
    float t     = clamp((linZ - uFogStart) / max(uFogEnd - uFogStart, 1e-4), 0.0, 1.0);
    FragColor = vec4(mix(scene, uFogColor, t), 1.0);
}
