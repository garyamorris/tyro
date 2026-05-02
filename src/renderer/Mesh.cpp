#include "Mesh.h"
#include "gl_loader.h"

#include <cmath>

namespace tyro {

namespace {
RenderStats g_stats;
}
RenderStats& renderStats() { return g_stats; }

void computeTangents(std::vector<Vertex>& verts,
                     const std::vector<uint32_t>& indices) {
  std::vector<Vec3> accum(verts.size(), Vec3{0,0,0});
  for (size_t i = 0; i + 2 < indices.size(); i += 3) {
    uint32_t i0 = indices[i], i1 = indices[i+1], i2 = indices[i+2];
    const Vec3& p0 = verts[i0].position;
    const Vec3& p1 = verts[i1].position;
    const Vec3& p2 = verts[i2].position;
    Vec2 u0 = verts[i0].uv, u1 = verts[i1].uv, u2 = verts[i2].uv;
    Vec3 e1 = p1 - p0;
    Vec3 e2 = p2 - p0;
    Vec2 d1 = u1 - u0;
    Vec2 d2 = u2 - u0;
    float denom = d1.x * d2.y - d2.x * d1.y;
    if (std::abs(denom) < 1e-8f) continue;
    float r = 1.0f / denom;
    Vec3 t {
      (d2.y * e1.x - d1.y * e2.x) * r,
      (d2.y * e1.y - d1.y * e2.y) * r,
      (d2.y * e1.z - d1.y * e2.z) * r,
    };
    accum[i0] = accum[i0] + t;
    accum[i1] = accum[i1] + t;
    accum[i2] = accum[i2] + t;
  }
  for (size_t i = 0; i < verts.size(); ++i) {
    Vec3 n = verts[i].normal;
    Vec3 t = accum[i];
    Vec3 ortho = t - n * dot(n, t);
    float L = length(ortho);
    verts[i].tangent = (L > 1e-6f) ? ortho / L : Vec3{1.0f, 0.0f, 0.0f};
  }
}

Mesh& Mesh::operator=(Mesh&& o) noexcept {
  if (this != &o) {
    destroy();
    vao_ = o.vao_; vbo_ = o.vbo_; ebo_ = o.ebo_; indexCount_ = o.indexCount_;
    o.vao_ = o.vbo_ = o.ebo_ = 0; o.indexCount_ = 0;
  }
  return *this;
}
Mesh::~Mesh() { destroy(); }
void Mesh::destroy() {
  if (ebo_) { glDeleteBuffers(1, &ebo_); ebo_ = 0; }
  if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
  if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
  indexCount_ = 0;
}

void Mesh::upload(const std::vector<Vertex>& verts,
                  const std::vector<uint32_t>& indices) {
  destroy();
  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  glGenBuffers(1, &ebo_);

  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(verts.size() * sizeof(Vertex)),
               verts.data(), GL_STATIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
               indices.data(), GL_STATIC_DRAW);

  // location 0: position
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void*)offsetof(Vertex, position));
  // location 1: normal
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void*)offsetof(Vertex, normal));
  // location 2: uv
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void*)offsetof(Vertex, uv));
  // location 3: tangent (default-initialised; only sampled by PBR-aware shaders)
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
                        (void*)offsetof(Vertex, tangent));

  glBindVertexArray(0);
  indexCount_ = indices.size();
}

void Mesh::draw() const {
  glBindVertexArray(vao_);
  glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indexCount_),
                 GL_UNSIGNED_INT, nullptr);
  glBindVertexArray(0);
  ++g_stats.drawCalls;
  g_stats.triangles += static_cast<int>(indexCount_ / 3);
}

FullscreenTriangle::~FullscreenTriangle() {
  if (vao_) glDeleteVertexArrays(1, &vao_);
}
void FullscreenTriangle::draw() {
  if (!vao_) glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glBindVertexArray(0);
}

} // namespace tyro
