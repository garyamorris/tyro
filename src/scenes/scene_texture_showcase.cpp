// Scene 7: TEXTURE LAB — 3x3 sphere grid showing GPU-textured (top row) vs
// procedural-pattern (mid) vs exotic (front) materials.

#include "../Demo.h"

namespace tyro {

void Demo::buildSceneTextureShowcase() {
  // Ground using the procedurally generated checker texture.
  Entity g; g.mesh = meshGround_; g.material = matTexChecker_;
  g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  // 3x3 grid of spheres demonstrating textured + procedural-pattern shaders.
  Material* row[3][3] = {
    { matTexBrick_,    matTexWood_,     matTexMarble_   }, // back: GPU textures
    { matMarbleProc_,  matWoodProc_,    matBrickProc_   }, // mid : procedural patterns
    { matHexProc_,     matIridescent_,  matHologram_    }, // front: exotic
  };
  const float spacing = 2.0f;
  for (int r = 0; r < 3; ++r) {
    for (int c = 0; c < 3; ++c) {
      Entity e;
      e.mesh = meshSphere_;
      e.localAABB = sphereAABB_;
      e.material = row[r][c];
      e.position = Vec3{ (c - 1) * spacing, 0.9f, (r - 1) * spacing };
      scene_.entities.push_back(e);
    }
  }

  // Lights — moderate sun + warm point light overhead.
  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.4f, -1.0f, -0.3f});
  sun.color = Vec3{1.0f, 0.96f, 0.88f}; sun.intensity = 0.95f;
  scene_.lights.push_back(sun);
  Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 0};
  p.color = Vec3{1.0f, 0.6f, 0.3f}; p.intensity = 2.2f; p.radius = 12.0f;
  scene_.lights.push_back(p);

  flyCam_.setLook(Vec3{0.0f, 2.5f, 5.5f}, Vec3{0, 1.0f, 0});
  scene_.camera.zFar = 100.0f;
}

} // namespace tyro
