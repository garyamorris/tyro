#pragma once
#include "math/Math.h"
#include "math/AABB.h"

namespace tyro {

class Mesh;
struct Material;

struct Entity {
  Mesh*     mesh     = nullptr;
  Material* material = nullptr;
  Vec3      position {0,0,0};
  Quat      rotation = Quat::identity();
  Vec3      scaling  {1,1,1};
  AABB      localAABB; // model-space AABB of the mesh

  Mat4 modelMatrix() const;
  AABB worldAABB()  const;
};

} // namespace tyro
