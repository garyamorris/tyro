#pragma once
#include "math/Math.h"
#include "math/AABB.h"

namespace tyro {

class Mesh;
struct Material;

// Entity — one drawable thing in a scene.
//
// Pairs a Mesh + Material with a TRS transform (position + rotation as quat
// + non-uniform scale) and the model-space AABB of the mesh. modelMatrix()
// composes the transform; worldAABB() applies it to the local AABB so
// frustum culling can work without touching per-vertex data.
//
// Entities are stored by value in `Scene::entities` and reference meshes /
// materials by raw pointer — those resources are owned (uniquely) by the
// Scene's vectors and outlive the entity.

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
