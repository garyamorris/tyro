// Scene 2: OCTREE STRESS — 512 entities scattered across an 18-unit cube.
// Toggle culling with F to compare visible counts; pair with B to watch
// the octree node bounds and entity AABBs blink in/out.

#include "../Demo.h"

#include <random>

namespace {
float frand(std::mt19937& rng, float lo, float hi) {
  std::uniform_real_distribution<float> d(lo, hi);
  return d(rng);
}
} // namespace

namespace tyro {

void Demo::buildSceneOctreeStress() {
  std::mt19937 rng(0xC0FFEEu);
  const int N = 512;
  const float R = 18.0f;

  Material* palette[6] = {
    matLitWarm_, matLitCool_, matToon_, matRim_, matChecker_, matUnlit_
  };
  Mesh* meshes[3] = { meshSphere_, meshCube_, meshTorus_ };
  AABB  boxes [3] = { sphereAABB_, cubeAABB_, torusAABB_ };

  for (int i = 0; i < N; ++i) {
    Entity e;
    int mi = std::uniform_int_distribution<int>(0, 2)(rng);
    e.mesh = meshes[mi];
    e.localAABB = boxes[mi];
    e.material = palette[std::uniform_int_distribution<int>(0, 5)(rng)];
    e.position = Vec3{ frand(rng,-R,R), frand(rng,-R*0.5f,R*0.5f), frand(rng,-R,R) };
    float s = frand(rng, 0.3f, 0.7f);
    e.scaling = Vec3{s,s,s};
    scene_.entities.push_back(e);
  }

  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.4f, -1.0f, 0.1f});
  sun.color = Vec3{1,1,1}; sun.intensity = 0.8f;
  scene_.lights.push_back(sun);
  Light p; p.type = LightType::Point;
  p.position = Vec3{0,5,0}; p.color = Vec3{1.0f,0.6f,0.3f};
  p.intensity = 3.0f; p.radius = 15.0f;
  scene_.lights.push_back(p);

  flyCam_.setLook(Vec3{ R, R*0.6f, R}, Vec3{0,0,0});
  scene_.camera.zFar = 200.0f;
}

} // namespace tyro
