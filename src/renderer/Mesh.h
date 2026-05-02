#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "math/Math.h"

namespace tyro {

struct RenderStats {
  int drawCalls = 0;
  int triangles = 0;
};
RenderStats& renderStats();

struct Vertex {
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
};

class Mesh {
public:
  Mesh() = default;
  ~Mesh();
  Mesh(const Mesh&) = delete;
  Mesh& operator=(const Mesh&) = delete;
  Mesh(Mesh&& o) noexcept { *this = static_cast<Mesh&&>(o); }
  Mesh& operator=(Mesh&& o) noexcept;

  void upload(const std::vector<Vertex>& verts,
              const std::vector<uint32_t>& indices);
  void draw() const;
  size_t indexCount() const { return indexCount_; }

private:
  unsigned int vao_ = 0, vbo_ = 0, ebo_ = 0;
  size_t indexCount_ = 0;
  void destroy();
};

// Returns a fullscreen-triangle "mesh" — actually a VAO with no buffer needed,
// driven by gl_VertexID in the shader. Caller binds and calls drawFullscreen().
class FullscreenTriangle {
public:
  FullscreenTriangle() = default;
  ~FullscreenTriangle();
  FullscreenTriangle(const FullscreenTriangle&) = delete;
  FullscreenTriangle& operator=(const FullscreenTriangle&) = delete;
  // VAO is created on first draw, so the type can be a member without a live GL context.
  void draw();
private:
  unsigned int vao_ = 0;
};

} // namespace tyro
