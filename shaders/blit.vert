#version 330 core
// Fullscreen triangle, no vertex buffer — positions derived from gl_VertexID.
//
// We emit three verts at (-1,-1), (3,-1), (-1,3). That triangle covers the
// entire NDC square (-1..1) and overshoots two corners, which the rasterizer
// clips away. Cheaper than a fullscreen quad: one triangle, no shared edge,
// no overdraw on the diagonal. Caller does glDrawArrays(GL_TRIANGLES, 0, 3)
// against an empty VAO.

out vec2 vUV;

void main() {
    vec2 p = vec2((gl_VertexID == 1) ? 3.0 : -1.0,
                  (gl_VertexID == 2) ? 3.0 : -1.0);
    vUV = (p + 1.0) * 0.5;
    gl_Position = vec4(p, 0.0, 1.0);
}
