// Scene 4: SHOWCASE BAY — teapot + Spot the cow + primitives, shadowed
// by warm/cool lights. The classic shadow-mapping demo.

#include "../Demo.h"

namespace tyro {

void Demo::buildSceneShowcase() {
  Entity g; g.mesh = meshGround_; g.material = matChecker_;
  g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc) {
    Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
    e.position = pos; e.scaling = sc;
    scene_.entities.push_back(e);
  };
  Vec3 teapotScale = meshTeapot_ ? fitScale(teapotAABB_, 2.5f) : Vec3{1,1,1};
  Vec3 spotScale   = meshSpot_   ? fitScale(spotAABB_,   1.6f) : Vec3{1,1,1};

  if (meshTeapot_) put(meshTeapot_, teapotAABB_, matLitWarm_, Vec3{-1.5f, 0.0f, 0.0f}, teapotScale);
  if (meshSpot_)   put(meshSpot_,   spotAABB_,   matLitCool_, Vec3{ 1.8f, 0.5f, 0.0f}, spotScale);
  put(meshSphere_, sphereAABB_, matRim_,     Vec3{ 0.0f, 0.5f,  2.5f}, Vec3{1,1,1});
  put(meshTorus_,  torusAABB_,  matToon_,    Vec3{-2.5f, 0.5f,  2.5f}, Vec3{1.4f,1.4f,1.4f});
  put(meshCube_,   cubeAABB_,   matRim_,     Vec3{ 2.5f, 0.5f, -2.5f}, Vec3{1,1,1});

  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.4f, -1.0f, -0.3f});
  sun.color = Vec3{1.0f, 0.95f, 0.85f}; sun.intensity = 0.9f;
  scene_.lights.push_back(sun);
  Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 0};
  p.color = Vec3{1.0f, 0.6f, 0.3f}; p.intensity = 2.0f; p.radius = 10.0f;
  scene_.lights.push_back(p);

  flyCam_.setLook(Vec3{0.0f, 2.5f, 6.0f}, Vec3{0,0.5f,0});
  scene_.camera.zFar = 100.0f;
}

} // namespace tyro
