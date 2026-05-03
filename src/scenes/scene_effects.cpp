// Scene 3: EFFECTS — small set of high-contrast hero meshes for showing
// off the post-process chain (cycle with P).

#include "../Demo.h"

namespace tyro {

void Demo::buildSceneEffects() {
  Entity g; g.mesh = meshGround_; g.material = matChecker_;
  g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc = Vec3{1,1,1}) {
    Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
    e.position = pos; e.scaling = sc;
    scene_.entities.push_back(e);
  };
  put(meshTorus_,    torusAABB_,  matRim_,      Vec3{-2.0f, 1.0f, 0.0f}, Vec3{2,2,2});
  put(meshSphere_,   sphereAABB_, matLitCool_,  Vec3{ 0.0f, 0.7f, 0.0f}, Vec3{1.4f,1.4f,1.4f});
  put(meshCube_,     cubeAABB_,   matLitWarm_,  Vec3{ 2.5f, 0.5f, 0.0f});
  put(meshSphere_,   sphereAABB_, matEmissive_, Vec3{-1.0f, 2.5f, -2.0f}, Vec3{0.5f,0.5f,0.5f});
  put(meshSphere_,   sphereAABB_, matEmissive_, Vec3{ 1.5f, 2.0f,  2.5f}, Vec3{0.4f,0.4f,0.4f});

  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.3f, -1.0f, -0.4f});
  sun.color = Vec3{0.9f, 0.85f, 0.7f}; sun.intensity = 0.6f;
  scene_.lights.push_back(sun);
  Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 0};
  p.color = Vec3{1.0f, 0.5f, 0.2f}; p.intensity = 4.0f; p.radius = 8.0f;
  scene_.lights.push_back(p);

  flyCam_.setLook(Vec3{ 0.0f, 2.0f, 6.5f}, Vec3{0,1,0});
  scene_.camera.zFar = 100.0f;
}

} // namespace tyro
