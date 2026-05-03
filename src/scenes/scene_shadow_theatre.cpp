// Scene 11: SHADOW THEATRE — four rotating movers cast sweeping shadows
// from a low warm sun across a clean checker plane and back wall. Pair
// with J (toggle shadows), L (frustum), M (depth thumbnail).
//
// tickActiveScene() yaws each mover at a different rate.

#include "../Demo.h"

namespace tyro {

void Demo::buildSceneShadowTheatre() {
  Entity g; g.mesh = meshGround_; g.material = matChecker_;
  g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  // Back wall — gives the shadows somewhere to climb when the sun is low.
  Entity wall; wall.mesh = meshCube_; wall.material = matLitCool_;
  wall.localAABB = cubeAABB_;
  wall.position = Vec3{0, 2.0f, -6.0f};
  wall.scaling  = Vec3{12.0f, 4.5f, 0.3f};
  scene_.entities.push_back(wall);

  // Four movers in a row — different mesh shapes give differently-shaped
  // shadows, so the ground reads as a mini animated theatre.
  shadowTheatreFirstMover_ = static_cast<int>(scene_.entities.size());
  Material* mats[4]   = { matPbrCopper_, matPbrGold_, matPbrPlastic_, matToon_ };
  Mesh*     meshes[4] = { meshCube_,     meshCylinder_, meshSphere_,  meshTorus_ };
  AABB      aabbs[4]  = { cubeAABB_,     cylAABB_,      sphereAABB_,  torusAABB_ };
  for (int i = 0; i < 4; ++i) {
    Entity e; e.mesh = meshes[i]; e.material = mats[i]; e.localAABB = aabbs[i];
    e.position = Vec3{(i - 1.5f) * 2.5f, 1.2f, 0.0f};
    e.scaling  = Vec3{0.7f, 0.7f, 0.7f};
    scene_.entities.push_back(e);
  }

  // Single low sun for long, dramatic shadows.
  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.6f, -0.7f, 0.3f});
  sun.color     = Vec3{1.0f, 0.95f, 0.85f};
  sun.intensity = 1.8f;
  scene_.lights.push_back(sun);

  flyCam_.setLook(Vec3{0, 3.5f, 7.5f}, Vec3{0, 1.0f, 0});
  scene_.camera.zFar = 100.0f;
}

} // namespace tyro
