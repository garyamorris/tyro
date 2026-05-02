#pragma once
#include "math/Math.h"

namespace tyro {

// AABB — axis-aligned bounding box (min/max corners, in some space).
//
// Built incrementally with expand(); transformed() applies a 4x4 using the
// "absolute-value-of-rotation" trick (Akenine-Möller, Real-Time Rendering)
// so the result stays axis-aligned even after the underlying object rotates.
// Used for entity bounds, octree node bounds, and frustum culling.

struct AABB {
  Vec3 min{0,0,0};
  Vec3 max{0,0,0};

  static AABB empty() {
    AABB b;
    b.min = Vec3{ 1e30f,  1e30f,  1e30f};
    b.max = Vec3{-1e30f, -1e30f, -1e30f};
    return b;
  }
  static AABB fromCenterHalf(Vec3 c, Vec3 h) {
    return { c - h, c + h };
  }

  Vec3 center()  const { return (min + max) * 0.5f; }
  Vec3 extents() const { return (max - min) * 0.5f; }
  Vec3 size()    const { return  max - min; }

  void expand(Vec3 p) {
    min.x = p.x < min.x ? p.x : min.x;
    min.y = p.y < min.y ? p.y : min.y;
    min.z = p.z < min.z ? p.z : min.z;
    max.x = p.x > max.x ? p.x : max.x;
    max.y = p.y > max.y ? p.y : max.y;
    max.z = p.z > max.z ? p.z : max.z;
  }
  void expand(const AABB& o) { expand(o.min); expand(o.max); }

  bool contains(const AABB& o) const {
    return o.min.x >= min.x && o.max.x <= max.x &&
           o.min.y >= min.y && o.max.y <= max.y &&
           o.min.z >= min.z && o.max.z <= max.z;
  }
  bool intersects(const AABB& o) const {
    return !(o.max.x < min.x || o.min.x > max.x ||
             o.max.y < min.y || o.min.y > max.y ||
             o.max.z < min.z || o.min.z > max.z);
  }

  // Transform by a 4x4: rotate the half-extents by absolute-value of the
  // rotation/scale 3x3, then translate the center. Standard "Akenine-Möller" trick.
  AABB transformed(const Mat4& M) const {
    Vec3 c = center();
    Vec3 e = extents();
    Vec3 newC{
      M.at(0,0)*c.x + M.at(0,1)*c.y + M.at(0,2)*c.z + M.at(0,3),
      M.at(1,0)*c.x + M.at(1,1)*c.y + M.at(1,2)*c.z + M.at(1,3),
      M.at(2,0)*c.x + M.at(2,1)*c.y + M.at(2,2)*c.z + M.at(2,3),
    };
    Vec3 newE{
      std::abs(M.at(0,0))*e.x + std::abs(M.at(0,1))*e.y + std::abs(M.at(0,2))*e.z,
      std::abs(M.at(1,0))*e.x + std::abs(M.at(1,1))*e.y + std::abs(M.at(1,2))*e.z,
      std::abs(M.at(2,0))*e.x + std::abs(M.at(2,1))*e.y + std::abs(M.at(2,2))*e.z,
    };
    return AABB::fromCenterHalf(newC, newE);
  }
};

} // namespace tyro
