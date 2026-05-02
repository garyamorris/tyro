#pragma once
#include "math/Math.h"
#include "math/AABB.h"

namespace tyro {

struct Plane {
  Vec3  n {0,1,0}; // unit normal
  float d  = 0.0f;  // n·x + d = 0 (so d = -n·p0)
};

// View-frustum: 6 planes pointing inward.
// Build with Frustum::fromViewProj(P * V).
struct Frustum {
  // L, R, B, T, N, F
  Plane planes[6];

  static Frustum fromViewProj(const Mat4& M) {
    auto row = [&](int r) {
      return Vec4{ M.at(r,0), M.at(r,1), M.at(r,2), M.at(r,3) };
    };
    Vec4 r0 = row(0), r1 = row(1), r2 = row(2), r3 = row(3);

    auto fromVec4 = [](Vec4 v) {
      Plane p; p.n = Vec3{v.x, v.y, v.z}; p.d = v.w;
      float L = length(p.n);
      if (L > 0.0f) { p.n = p.n / L; p.d /= L; }
      return p;
    };

    Frustum f;
    f.planes[0] = fromVec4(r3 + r0); // left
    f.planes[1] = fromVec4(Vec4{r3.x - r0.x, r3.y - r0.y, r3.z - r0.z, r3.w - r0.w}); // right
    f.planes[2] = fromVec4(r3 + r1); // bottom
    f.planes[3] = fromVec4(Vec4{r3.x - r1.x, r3.y - r1.y, r3.z - r1.z, r3.w - r1.w}); // top
    f.planes[4] = fromVec4(r3 + r2); // near
    f.planes[5] = fromVec4(Vec4{r3.x - r2.x, r3.y - r2.y, r3.z - r2.z, r3.w - r2.w}); // far
    return f;
  }

  // Returns false only when the box is completely outside one of the planes.
  bool intersects(const AABB& box) const {
    for (int i = 0; i < 6; ++i) {
      const Vec3& n = planes[i].n;
      // p-vertex: corner of the box farthest along the plane normal.
      Vec3 p{
        n.x >= 0.0f ? box.max.x : box.min.x,
        n.y >= 0.0f ? box.max.y : box.min.y,
        n.z >= 0.0f ? box.max.z : box.min.z,
      };
      if (dot(n, p) + planes[i].d < 0.0f) return false;
    }
    return true;
  }
};

} // namespace tyro
