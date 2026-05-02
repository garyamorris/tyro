#include "DebugDraw.h"

#include "Shader.h"
#include "gl_loader.h"

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
