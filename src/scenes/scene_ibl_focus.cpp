// Scene 13: IBL FOCUS — five gold metallic spheres with roughness sweeping
// 0..1, lit by NOTHING but IBL. With K off the spheres are almost pitch
// black (3% ambient); with K on they pop with prefiltered environment
// reflections that smear smoothly with roughness.
//
// Cleanest possible teaching scene for the Karis 2014 split-sum, isolated
// from any Cook-Torrance direct term.

#include "../Demo.h"

#include <memory>

namespace tyro {

void Demo::buildSceneIblFocus() {
  // Dim grey floor — keep it dark so the spheres dominate.
  auto floorMat = std::make_unique<Material>();
  floorMat->shader    = shPbr_;
  floorMat->albedo    = Vec3{0.20f, 0.20f, 0.22f};
  floorMat->metallic  = 0.0f;
  floorMat->roughness = 0.85f;
  Material* floorPtr = floorMat.get();
  scene_.materials.push_back(std::move(floorMat));

  Entity g; g.mesh = meshGround_; g.material = floorPtr;
  g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  // Row of 5 metallic spheres, gold albedo, roughness 0..1.
  const int N = 5;
  const float spacing = 1.6f;
  for (int i = 0; i < N; ++i) {
    auto m = std::make_unique<Material>();
    m->shader    = shPbr_;
    m->albedo    = Vec3{0.95f, 0.78f, 0.40f};
    m->metallic  = 1.0f;
    m->roughness = float(i) / float(N - 1);
    Material* mp = m.get();
    scene_.materials.push_back(std::move(m));

    Entity e; e.mesh = meshSphere_; e.material = mp; e.localAABB = sphereAABB_;
    e.position = Vec3{(i - (N-1)*0.5f) * spacing, 0.7f, 0.0f};
    e.scaling  = Vec3{0.7f, 0.7f, 0.7f};
    scene_.entities.push_back(e);
  }

  // Intentionally NO Light entries — this scene is the IBL contribution
  // in isolation. Press K to compare with-IBL vs without-IBL.

  flyCam_.setLook(Vec3{0, 1.6f, 5.5f}, Vec3{0, 0.7f, 0});
  scene_.camera.zFar = 60.0f;
}

} // namespace tyro
