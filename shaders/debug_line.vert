#version 330 core
// Debug line vertex shader.
// Transforms world-space line endpoints into clip space and forwards the
// per-vertex colour. Used by DebugDraw for entity AABBs, octree node
// bounds, and any other line gizmo overlays.

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aCol;

uniform mat4 uViewProj;

out vec3 vCol;

void main() {
  vCol = aCol;
  gl_Position = uViewProj * vec4(aPos, 1.0);
}
