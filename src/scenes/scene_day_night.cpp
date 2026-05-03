// Scene 12: DAY/NIGHT CYCLE — sun arcs around the X-Y plane every 24 s.
// tickActiveScene() updates the sun's direction / colour / intensity and
// ramps the four corner torches on as dusk arrives.

#include "../Demo.h"

namespace tyro {

void Demo::buildSceneDayNight() {
  Entity g; g.mesh = meshGround_; g.material = matPbrBrick_;
  g.position = Vec3{0, -0.5f, 0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc = Vec3{1,1,1}) {
    Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
    e.position = pos; e.scaling = sc;
    scene_.entities.push_back(e);
  };
  put(meshSphere_, sphereAABB_, matPbrCopper_,  Vec3{-3.0f, 0.7f, 0.0f}, Vec3{0.8f,0.8f,0.8f});
  put(meshTorus_,  torusAABB_,  matPbrGold_,    Vec3{ 0.0f, 1.0f, 0.0f}, Vec3{1.4f,1.4f,1.4f});
  put(meshCube_,   cubeAABB_,   matPbrPlastic_, Vec3{ 3.0f, 0.7f, 0.0f}, Vec3{0.9f,0.9f,0.9f});

  // Sun: lights[0] so the shadow pass picks it up. Direction + intensity
  // are overwritten every frame in tickActiveScene; initial values are
  // "noon" so the first frame already looks correct.
  Light sun; sun.type = LightType::Directional;
  sun.direction = Vec3{0.0f, -1.0f, 0.0f};
  sun.color     = Vec3{1.0f, 0.95f, 0.85f};
  sun.intensity = 1.5f;
  scene_.lights.push_back(sun);

  // Four corner torches; intensity ramps in tickActiveScene.
  dayNightTorchStart_ = static_cast<int>(scene_.lights.size());
  Vec3 torchPos[4] = {
    {-6.0f, 1.5f, -6.0f}, { 6.0f, 1.5f, -6.0f},
    {-6.0f, 1.5f,  6.0f}, { 6.0f, 1.5f,  6.0f},
  };
  for (int i = 0; i < 4; ++i) {
    Light t; t.type = LightType::Point;
    t.position  = torchPos[i];
    t.color     = Vec3{1.0f, 0.55f, 0.20f};
    t.intensity = 0.0f;       // tick fills this in
    t.radius    = 8.0f;
    scene_.lights.push_back(t);
  }

  flyCam_.setLook(Vec3{0, 3.0f, 8.0f}, Vec3{0, 1.0f, 0});
  scene_.camera.zFar = 120.0f;
}

} // namespace tyro
