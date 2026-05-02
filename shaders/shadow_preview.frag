#version 330 core
// Visualises the directional sun's shadow depth texture as a grayscale
// thumbnail. Pairs with blit.vert (fullscreen triangle, gl_VertexID-driven).
//
// The depth texture stores values in [0,1] with 0 = near plane, 1 = far
// plane. For an orthographic light that's roughly linear in world space,
// so a darker pixel = closer to the light.
//
// We also draw a yellow border (matching the wire-frustum overlay's colour)
// so when both M and L are toggled on the user can see the pairing at a
// glance.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uDepth;

void main() {
    float d = texture(uDepth, vUV).r;

    // 1 px border at the very edge in NDC space.
    vec2  e      = abs(vUV - 0.5) * 2.0;     // 0 at center, 1 at edges
    float border = step(0.985, max(e.x, e.y));
    vec3  col    = mix(vec3(d), vec3(1.0, 0.95, 0.4), border);

    FragColor = vec4(col, 1.0);
}
