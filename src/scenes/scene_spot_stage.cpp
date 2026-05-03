// Scene 10: SPOTLIGHT STAGE — three coloured spots converge on a central
// hero. Press L to see the cones, K for IBL fill, 1-7 to swap material on
// every entity (the cone math is shared across BRDFs).

#include "../Demo.h"

#include <cmath>
#include <memory>

namespace tyro {

void Demo::buildSceneSpotStage() {
  // Dark matte floor — high contrast against the coloured spots.
  auto darkMat = std::make_unique<Material>();
  darkMat->shader = shLit_;
  darkMat->albedo = Vec3{0.06f, 0.06f, 0.07f};
  Material* dark = darkMat.get();
  scene_.materials.push_back(std::move(darkMat));

  Entity g; g.mesh = meshGround_; g.material = dark;
  g.position = Vec3{0, -0.5f, 0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  // Wood pedestal + iridescent hero sphere on top.
  auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc = Vec3{1,1,1}) {
    Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
    e.position = pos; e.scaling = sc;
    scene_.entities.push_back(e);
  };
  put(meshCube_,   cubeAABB_,   matPbrWood_,    Vec3{0, 0.4f, 0}, Vec3{0.6f, 0.4f, 0.6f});
  put(meshSphere_, sphereAABB_, matIridescent_, Vec3{0, 1.4f, 0}, Vec3{0.55f, 0.55f, 0.55f});

  // Three spots at 120° intervals, all aimed at the hero. Tight cones so
  // the colour pools on the floor are clearly separated.
  Vec3 colors[3] = {
    {1.00f, 0.25f, 0.35f},  // red
    {0.25f, 1.00f, 0.40f},  // green
    {0.30f, 0.45f, 1.00f},  // blue
  };
  Vec3 target{0.0f, 1.4f, 0.0f};
  for (int i = 0; i < 3; ++i) {
    float a = (float(i) / 3.0f) * 2.0f * kPi;
    Vec3 pos{std::cos(a) * 5.5f, 5.0f, std::sin(a) * 5.5f};
    Light s;
    s.type      = LightType::Spot;
    s.position  = pos;
    s.direction = normalize(target - pos);
    s.color     = colors[i];
    s.intensity = 9.0f;
    s.radius    = 9.0f;
    s.innerDeg  = 8.0f;
    s.outerDeg  = 14.0f;
    scene_.lights.push_back(s);
  }

  flyCam_.setLook(Vec3{0, 2.5f, 7.0f}, Vec3{0, 1.4f, 0});
  scene_.camera.zFar = 80.0f;
}

} // namespace tyro
