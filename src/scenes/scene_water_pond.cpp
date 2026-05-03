// Scene 5: WATER POND — animated water plane (sin-sum vertex displacement
// + Fresnel) with floating teapot/cow. tickActiveScene() bobs the floats.

#include "../Demo.h"

#include <cmath>

namespace tyro {

void Demo::buildSceneWaterPond() {
  // Water — large displaced plane, water material.
  Entity w; w.mesh = meshWater_; w.material = matWater_;
  w.position = Vec3{0, 0, 0}; w.localAABB = waterAABB_;
  waterEntityIdx_ = static_cast<int>(scene_.entities.size());
  scene_.entities.push_back(w);

  auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc) {
    Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
    e.position = pos; e.scaling = sc;
    scene_.entities.push_back(e);
  };

  Vec3 teapotScale = meshTeapot_ ? fitScale(teapotAABB_, 1.8f) : Vec3{1,1,1};
  Vec3 spotScale   = meshSpot_   ? fitScale(spotAABB_,   1.2f) : Vec3{1,1,1};

  waterFloatStart_ = static_cast<int>(scene_.entities.size());
  if (meshTeapot_) put(meshTeapot_, teapotAABB_, matLitWarm_, Vec3{-1.0f, 0.6f,  0.0f}, teapotScale);
  if (meshSpot_)   put(meshSpot_,   spotAABB_,   matLitCool_, Vec3{ 1.5f, 0.5f, -1.0f}, spotScale);
  // Stones around the pond edge — small dark cubes.
  for (int i = 0; i < 6; ++i) {
    float a = (float(i) / 6.0f) * 2.0f * kPi;
    Vec3 pos{std::cos(a) * 7.0f, -0.1f, std::sin(a) * 7.0f};
    Vec3 sc{0.6f + (i*0.07f), 0.3f, 0.5f};
    put(meshCube_, cubeAABB_, matLitWarm_, pos, sc);
  }

  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.3f, -0.8f, -0.5f});
  sun.color = Vec3{1.0f, 0.92f, 0.78f}; sun.intensity = 1.0f;
  scene_.lights.push_back(sun);

  flyCam_.setLook(Vec3{ 0.0f, 1.5f, 7.0f}, Vec3{0, 0.3f, 0});
  scene_.camera.zFar = 120.0f;
}

} // namespace tyro
