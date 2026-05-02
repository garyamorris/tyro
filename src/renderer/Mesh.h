#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

#include "math/Math.h"

namespace tyro {

// Mesh — VAO + VBO + EBO bundle for indexed triangle geometry.
//
// Vertex layout is fixed: position + normal + uv + tangent (44 bytes).
// upload() ships verts/indices to GPU; draw() binds the VAO, issues
// glDrawElements, and bumps the per-frame counters in renderStats() (which
// the on-screen stats overlay reads).
//
// computeTangents() (free function) derives per-vertex tangents from
// triangle UV gradients (Möller 1996), Gram-Schmidt-orthogonalised against
// the smooth normal. Primitives and the OBJ loader call it so PBR normal
// mapping has a usable TBN basis.
//
// FullscreenTriangle is a degenerate "mesh" with no buffer — vertices are
// fabricated from gl_VertexID inside blit.vert. Used for every post-FX pass
// and the IBL BRDF LUT bake.

struct RenderStats {
  int drawCalls = 0;
  int triangles = 0;
};
RenderStats& renderStats();

struct Vertex {
  Vec3 position;
  Vec3 normal;
  Vec2 uv;
  Vec3 tangent { 1.0f, 0.0f, 0.0f };  // default placeholder; computeTangents fills it
};

// Recompute tangents per vertex from triangle UV gradients (MOLLER 1996).
// Result is Gram-Schmidt orthogonalised against the existing smooth normal.
void computeTangents(std::vector<Vertex>& verts,
                     const std::vector<uint32_t>& indices);

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
