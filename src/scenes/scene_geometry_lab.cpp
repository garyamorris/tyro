// Scene 6: GEOMETRY LAB — three meshes pulsing via the explode geometry
// shader (face-normal displacement). Sets uMagnitude on the explode shader
// at scene-build time; persists for the duration of this scene.

#include "../Demo.h"

namespace tyro {

void Demo::buildSceneGeometryLab() {
  Entity g; g.mesh = meshGround_; g.material = matChecker_;
  g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc) {
    Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
    e.position = pos; e.scaling = sc;
    scene_.entities.push_back(e);
  };
  // Three "lab specimens" using the explode geom shader — pulses outward
  // along each face's normal so you can see the underlying tessellation.
  Vec3 teapotScale = meshTeapot_ ? fitScale(teapotAABB_, 2.5f) : Vec3{1,1,1};
  Vec3 spotScale   = meshSpot_   ? fitScale(spotAABB_,   1.8f) : Vec3{1,1,1};

  if (meshTeapot_) put(meshTeapot_, teapotAABB_, matExplode_, Vec3{-2.5f, 0.6f, 0.0f}, teapotScale);
  if (meshSpot_)   put(meshSpot_,   spotAABB_,   matExplode_, Vec3{ 0.0f, 0.7f, 0.0f}, spotScale);
  put(meshTorus_,  torusAABB_,  matExplode_, Vec3{ 2.5f, 0.7f, 0.0f}, Vec3{1.6f,1.6f,1.6f});

  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.4f, -1.0f, -0.3f});
  sun.color = Vec3{1,1,1}; sun.intensity = 0.9f;
  scene_.lights.push_back(sun);
  Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 2};
  p.color = Vec3{0.4f, 0.9f, 1.0f}; p.intensity = 2.5f; p.radius = 12.0f;
  scene_.lights.push_back(p);

  // Set magnitude on the explode shader. Stays for the duration of this scene.
  shExplode_->bind();
  shExplode_->setFloat("uMagnitude", 0.25f);

  flyCam_.setLook(Vec3{0.0f, 2.0f, 5.5f}, Vec3{0, 0.5f, 0});
  scene_.camera.zFar = 100.0f;
}

} // namespace tyro
