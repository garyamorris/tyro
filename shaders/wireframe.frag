#version 330 core

// Barycentric wireframe: per-fragment distance to the nearest triangle edge,
// scaled by fwidth() so the line thickness is constant in screen space.

in  vec3 vBary;
out vec4 FragColor;

uniform vec3  uWireColor;   // edge color
uniform vec3  uFillColor;   // interior color (used when uFillAlpha > 0)
uniform float uFillAlpha;   // 0 = overlay (interior transparent), 1 = solid
uniform float uThickness;   // in pixels

void main() {
    vec3 d = fwidth(vBary);
    vec3 a = smoothstep(vec3(0.0), d * uThickness, vBary);
    float edge = min(min(a.x, a.y), a.z); // 0 at edge, 1 at interior

    vec3  color = mix(uWireColor, uFillColor, edge);
    float alpha = mix(1.0,        uFillAlpha, edge);
    if (alpha < 0.02) discard;
    FragColor = vec4(color, alpha);
}
