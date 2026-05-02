#include "DebugDraw.h"

#include "Shader.h"
#include "gl_loader.h"

#include <cmath>
#include <iterator>

namespace tyro {

DebugDraw::~DebugDraw() { destroy(); }

bool DebugDraw::init() {
  shader_ = new Shader();
  if (!shader_->loadFromFiles("shaders/debug_line.vert",
                              "shaders/debug_line.frag")) {
    delete shader_; shader_ = nullptr;
    return false;
  }

  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  // pos(3) + col(3) = 6 floats per vertex, stride 24.
  const GLsizei stride = 6 * sizeof(float);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride,
                        (void*)(3 * sizeof(float)));
  glBindVertexArray(0);
  return true;
}

void DebugDraw::destroy() {
  if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
  if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
  if (shader_) { delete shader_; shader_ = nullptr; }
}

void DebugDraw::reloadShaderIfChanged() {
  if (shader_) shader_->reloadIfChanged();
}

void DebugDraw::line(Vec3 a, Vec3 b, Vec3 color) {
  float seg[12] = {
    a.x, a.y, a.z, color.x, color.y, color.z,
    b.x, b.y, b.z, color.x, color.y, color.z,
  };
  verts_.insert(verts_.end(), std::begin(seg), std::end(seg));
}

void DebugDraw::aabb(const AABB& box, Vec3 color) {
  // Eight corners indexed by bits: x=bit 0, y=bit 1, z=bit 2.
  Vec3 c[8];
  for (int i = 0; i < 8; ++i) {
    c[i].x = (i & 1) ? box.max.x : box.min.x;
    c[i].y = (i & 2) ? box.max.y : box.min.y;
    c[i].z = (i & 4) ? box.max.z : box.min.z;
  }
  // 12 edges: bottom (y=min) face, top (y=max) face, four verticals.
  static const int edges[12][2] = {
    {0,1},{1,5},{5,4},{4,0},   // bottom
    {2,3},{3,7},{7,6},{6,2},   // top
    {0,2},{1,3},{4,6},{5,7},   // verticals
  };
  for (auto& e : edges) line(c[e[0]], c[e[1]], color);
}

void DebugDraw::sphere(Vec3 c, float r, Vec3 color, int segments) {
  if (segments < 3) segments = 3;
  // Three orthogonal great circles in the XY, XZ, and YZ planes.
  for (int axis = 0; axis < 3; ++axis) {
    Vec3 prev{};
    for (int i = 0; i <= segments; ++i) {
      float t = (2.0f * kPi * float(i)) / float(segments);
      float ca = std::cos(t) * r;
      float sa = std::sin(t) * r;
      Vec3 p;
      if      (axis == 0) p = c + Vec3{ca, sa, 0.0f};   // XY plane
      else if (axis == 1) p = c + Vec3{ca, 0.0f, sa};   // XZ plane
      else                p = c + Vec3{0.0f, ca, sa};   // YZ plane
      if (i > 0) line(prev, p, color);
      prev = p;
    }
  }
}

void DebugDraw::wireFrustum(const Vec3 corners[8], Vec3 color) {
  // 12 edges: 4 on the near face, 4 on the far face, 4 connectors.
  static const int edges[12][2] = {
    {0,1},{1,2},{2,3},{3,0},   // near face
    {4,5},{5,6},{6,7},{7,4},   // far face
    {0,4},{1,5},{2,6},{3,7},   // near→far connectors
  };
  for (auto& e : edges) line(corners[e[0]], corners[e[1]], color);
}

void DebugDraw::cone(Vec3 apex, Vec3 axis, float halfAngleDeg, float length,
                     Vec3 color, int segments) {
  if (segments < 4) segments = 4;
  Vec3 a = normalize(axis);
  float radius = length * std::tan(halfAngleDeg * kPi / 180.0f);
  Vec3 baseCenter = apex + a * length;

  // Build a basis perpendicular to the axis. The fallback handles the
  // degenerate case where axis ≈ ±Y.
  Vec3 worldUp = (std::abs(a.y) > 0.99f) ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
  Vec3 right   = normalize(cross(a, worldUp));
  Vec3 up      = cross(right, a);

  // Walk the base circle as line segments, and emit generator rays from
  // apex on a fixed cadence so the cone reads as 3D from any angle.
  int rayEvery = std::max(1, segments / 4);
  Vec3 prev{};
  for (int i = 0; i <= segments; ++i) {
    float t = (2.0f * kPi * float(i)) / float(segments);
    Vec3 p = baseCenter + (right * std::cos(t) + up * std::sin(t)) * radius;
    if (i > 0) line(prev, p, color);
    if (i % rayEvery == 0) line(apex, p, color);
    prev = p;
  }
}

void DebugDraw::flush(const Mat4& viewProj) {
  if (verts_.empty() || !shader_) return;

  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(verts_.size() * sizeof(float)),
               verts_.data(), GL_DYNAMIC_DRAW);

  shader_->bind();
  shader_->setMat4("uViewProj", viewProj);

  GLsizei n = static_cast<GLsizei>(verts_.size() / 6);
  glDrawArrays(GL_LINES, 0, n);

  glBindVertexArray(0);
  verts_.clear();
}

} // namespace tyro
