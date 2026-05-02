#version 330 core
// Debug line fragment shader.
// Outputs the interpolated per-vertex colour directly. The framebuffer is
// HDR (RGBA16F) — the post-FX final pass owns tonemap + gamma — so writing
// linear colour through here is correct.

in  vec3 vCol;
out vec4 FragColor;

void main() {
  FragColor = vec4(vCol, 1.0);
}
