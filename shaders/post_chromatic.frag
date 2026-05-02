#version 330 core
// Chromatic aberration.
// Sample R, G, B at slightly different offsets along the radial direction
// (vUV - 0.5), so the R channel is pulled inward and B outward (or vice
// versa). uAmount scales the displacement. Mimics the wavelength-dependent
// dispersion that real lenses get worst at the frame edges.

in  vec2 vUV;
out vec4 FragColor;

uniform sampler2D uColor;
uniform float     uAmount;

void main() {
    vec2 dir = vUV - vec2(0.5);
    float r = texture(uColor, vUV - dir * uAmount).r;
    float g = texture(uColor, vUV).g;
    float b = texture(uColor, vUV + dir * uAmount).b;
    FragColor = vec4(r, g, b, 1.0);
}
