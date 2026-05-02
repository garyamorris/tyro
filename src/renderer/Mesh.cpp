#include "Mesh.h"
#include "gl_loader.h"

namespace tyro {

namespace {
RenderStats g_stats;
}
RenderStats& renderStats() { return g_stats; }

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
