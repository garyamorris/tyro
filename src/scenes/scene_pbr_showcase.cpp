// Scene 8: PBR LAB — 3x5 sphere grid: metallic sweep (back), roughness
// sweep at metallic=1 (mid), textured PBR materials (front). Cook-Torrance
// GGX, normal mapping, ACES tonemap, IBL ambient + skybox.

#include "../Demo.h"

#include <memory>

namespace tyro {

void Demo::buildScenePbrShowcase() {
  // Ground plane sampled with the textured PBR brick.
  Entity g; g.mesh = meshGround_; g.material = matPbrBrick_;
  g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  auto putSphere = [&](Material* m, Vec3 pos, Vec3 sc = Vec3{0.7f, 0.7f, 0.7f}) {
    Entity e; e.mesh = meshSphere_; e.material = m; e.localAABB = sphereAABB_;
    e.position = pos; e.scaling = sc;
    scene_.entities.push_back(e);
  };

  const float spacing = 1.5f;
  const int N = 5;

  // Row 0 (back): metallic sweep at fixed roughness=0.30, gold tint.
  for (int i = 0; i < N; ++i) {
    float metallic = float(i) / float(N - 1);
    auto m = std::make_unique<Material>();
    m->shader = shPbr_;
    m->albedo = Vec3{1.00f, 0.78f, 0.34f};
    m->metallic = metallic;
    m->roughness = 0.30f;
    m->normalTex = texNormalRough_;
    m->uvScale = Vec2{2, 2};
    Material* mp = m.get();
    scene_.materials.push_back(std::move(m));
    putSphere(mp, Vec3{(i - (N-1)*0.5f) * spacing, 0.7f, -spacing});
  }

  // Row 1 (mid): roughness sweep at metallic=1, copper tint.
  for (int i = 0; i < N; ++i) {
    float roughness = 0.05f + (1.0f - 0.05f) * (float(i) / float(N - 1));
    auto m = std::make_unique<Material>();
    m->shader = shPbr_;
    m->albedo = Vec3{0.95f, 0.64f, 0.54f};
    m->metallic = 1.0f;
    m->roughness = roughness;
    m->normalTex = texNormalRough_;
    m->uvScale = Vec2{2, 2};
    Material* mp = m.get();
    scene_.materials.push_back(std::move(m));
    putSphere(mp, Vec3{(i - (N-1)*0.5f) * spacing, 0.7f, 0.0f});
  }

  // Row 2 (front): textured PBR + uniform-color showcase.
  Material* front[5] = { matPbrBrick_, matPbrWood_, matPbrMarble_, matPbrPlastic_, matPbrGold_ };
  for (int i = 0; i < 5; ++i) {
    putSphere(front[i], Vec3{(i - 2) * spacing, 0.7f, spacing});
  }

  // Lights — a sun for shadow + a warm fill point.
  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.4f, -1.0f, -0.3f});
  sun.color = Vec3{1.0f, 0.96f, 0.88f}; sun.intensity = 1.1f;
  scene_.lights.push_back(sun);
  Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 2};
  p.color = Vec3{1.0f, 0.7f, 0.4f}; p.intensity = 2.5f; p.radius = 12.0f;
  scene_.lights.push_back(p);

  flyCam_.setLook(Vec3{0.0f, 2.5f, 5.5f}, Vec3{0, 0.7f, 0});
  scene_.camera.zFar = 100.0f;
}

} // namespace tyro
