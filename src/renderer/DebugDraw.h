#pragma once
#include <vector>

#include "math/Math.h"
#include "math/AABB.h"

namespace tyro {

class Shader;

// DebugDraw — immediate-mode batched line renderer.
//
// Push 3D line segments and AABB outlines from any code path; flush() issues
// a single GL_LINES draw call. Mirrors TextRenderer's pattern (vector
// accumulator + dynamic VBO + single per-frame flush) and Skybox's
// pattern (owns its shader, supports hot-reload).
//
// Currently used for entity world AABBs and octree node bounds — but the
// API is general; future visualisations (light radii, frustum corners,
// arbitrary line gizmos) can plug in without changes here.
class DebugDraw {
public:
  DebugDraw() = default;
  ~DebugDraw();
  DebugDraw(const DebugDraw&) = delete;
  DebugDraw& operator=(const DebugDraw&) = delete;

  bool init();
  void destroy();

  void reloadShaderIfChanged();

  // Accumulators — call any number of times per frame.
  void line(Vec3 a, Vec3 b, Vec3 color);
  void aabb(const AABB& box, Vec3 color);   // pushes the 12 edges

  // Three orthogonal great circles. `segments` controls how round the
  // sphere looks (24 ≈ visibly round, 12 ≈ low-poly).
  void sphere(Vec3 center, float radius, Vec3 color, int segments = 24);

  // Wireframe of an arbitrary frustum given its 8 world-space corners in the
  // canonical order: 0..3 = near face (BL, BR, TR, TL), 4..7 = far face,
  // same winding. Used to visualise the directional-light shadow VP.
  void wireFrustum(const Vec3 corners[8], Vec3 color);

  // Single batched draw call. Caller controls depth-test state — this just
  // binds the shader and draws. Clears the accumulator.
  void flush(const Mat4& viewProj);

private:
  // 6 floats per vertex: pos.xyz, col.rgb (matches TextRenderer's flat-float
  // vector approach so the GL setup looks the same).
  std::vector<float> verts_;
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
  Shader*      shader_ = nullptr;  // owned (matches Skybox::shader_)
};

} // namespace tyro
