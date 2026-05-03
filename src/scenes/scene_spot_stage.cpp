// Scene 10: SPOTLIGHT STAGE — three coloured spots converge on a central
// hero. Press L to see the cones, K for IBL fill, 1-7 to swap material on
// every entity (the cone math is shared across BRDFs).

#include "../Demo.h"

#include <cmath>
#include <memory>

namespace tyro {

void Demo::buildSceneSpotStage() {
  // Dark matte floor — high contrast against the coloured spots, but not so
  // dark that the colour pools die in the (1 - d/r)² attenuation tail.
  auto darkMat = std::make_unique<Material>();
  darkMat->shader = shLit_;
  darkMat->albedo = Vec3{0.12f, 0.12f, 0.14f};
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
  // Geometry note: each spot sits at y=5, ~5.5 from the centre, aimed at
  // the orb at y=1.4. The cone axis hits the floor (y=-0.5) at ~10 units
  // along the axis, so `radius` MUST exceed that or `(1 - d/r)²` clamps
  // to zero before the cone reaches the ground and the floor stays black.
  // Picking 14 leaves a comfortable falloff tail past the floor pool.
  // Cones are also widened slightly (12°/20°) so the floor pools read.
  Vec3 target{0.0f, 1.4f, 0.0f};
  for (int i = 0; i < 3; ++i) {
    float a = (float(i) / 3.0f) * 2.0f * kPi;
    Vec3 pos{std::cos(a) * 5.5f, 5.0f, std::sin(a) * 5.5f};
    Light s;
    s.type      = LightType::Spot;
    s.position  = pos;
    s.direction = normalize(target - pos);
    s.color     = colors[i];
    s.intensity = 12.0f;
    s.radius    = 14.0f;
    s.innerDeg  = 12.0f;
    s.outerDeg  = 20.0f;
    scene_.lights.push_back(s);
  }

  flyCam_.setLook(Vec3{0, 2.5f, 7.0f}, Vec3{0, 1.4f, 0});
  scene_.camera.zFar = 80.0f;
}

} // namespace tyro
