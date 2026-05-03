// Scene 1: LIGHT GARDEN — 6 colored point lights orbit a checker plane.
// tickActiveScene() updates each light's position + its emissive marker
// sphere every frame.

#include "../Demo.h"

#include <memory>

namespace tyro {

void Demo::buildSceneLightGarden() {
  Entity g; g.mesh = meshGround_; g.material = matChecker_;
  g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  // Hero objects.
  auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc = Vec3{1,1,1}) {
    Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
    e.position = pos; e.scaling = sc;
    scene_.entities.push_back(e);
  };
  put(meshTorus_,    torusAABB_,  matLitWarm_, Vec3{ 0.0f, 1.0f, 0.0f}, Vec3{2,2,2});
  put(meshSphere_,   sphereAABB_, matLitCool_, Vec3{-3.0f, 0.5f, 0.0f});
  put(meshSphere_,   sphereAABB_, matToon_,    Vec3{ 3.0f, 0.5f, 0.0f});
  put(meshCube_,     cubeAABB_,   matRim_,     Vec3{ 0.0f, 0.5f,-3.0f});
  put(meshCylinder_, cylAABB_,    matLitWarm_, Vec3{ 0.0f, 0.5f, 3.0f});

  // 6 orbiting point lights, each visualised by a tiny emissive sphere.
  Vec3 colors[6] = {
    {1.0f, 0.3f, 0.3f}, {0.3f, 1.0f, 0.3f}, {0.3f, 0.3f, 1.0f},
    {1.0f, 1.0f, 0.3f}, {1.0f, 0.3f, 1.0f}, {0.3f, 1.0f, 1.0f},
  };
  lightGardenStart_ = static_cast<int>(scene_.entities.size());
  for (int i = 0; i < 6; ++i) {
    Light L; L.type = LightType::Point;
    L.position  = Vec3{0,1,0};
    L.color     = colors[i];
    L.intensity = 2.5f;
    L.radius    = 6.0f;
    scene_.lights.push_back(L);

    // Marker sphere — unlit + emissive matching light color.
    auto m = std::make_unique<Material>();
    m->shader = shUnlit_;
    m->albedo = colors[i];
    m->emissive = colors[i] * 1.2f;
    Material* mp = m.get();
    scene_.materials.push_back(std::move(m));

    Entity e; e.mesh = meshSphere_; e.material = mp; e.localAABB = sphereAABB_;
    e.position = L.position; e.scaling = Vec3{0.18f, 0.18f, 0.18f};
    scene_.entities.push_back(e);
  }

  // Dim sun for ambient fill.
  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.2f, -1.0f, -0.1f});
  sun.color = Vec3{0.5f, 0.6f, 0.8f}; sun.intensity = 0.25f;
  scene_.lights.push_back(sun);

  flyCam_.setLook(Vec3{0.0f, 4.0f, 8.0f}, Vec3{0,1,0});
  scene_.camera.zFar = 120.0f;
}

} // namespace tyro
