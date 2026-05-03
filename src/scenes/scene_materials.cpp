// Scene 0: MATERIALS — 5 mesh types x 4 materials grid showing how the same
// geometry looks under different BRDFs (phong warm/cool, toon, rim).

#include "../Demo.h"

namespace tyro {

void Demo::buildSceneMaterials() {
  // Ground.
  Entity g; g.mesh = meshGround_; g.material = matChecker_;
  g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
  scene_.entities.push_back(g);

  Mesh*    meshes[5] = { meshCube_, meshSphere_, meshTorus_, meshCylinder_, meshPlane_ };
  AABB     boxes [5] = { cubeAABB_, sphereAABB_, torusAABB_, cylAABB_,      planeAABB_ };
  Material* mats [4] = { matLitWarm_, matLitCool_, matToon_, matRim_ };
  const float xStep = 1.6f, zStep = 1.8f;
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 5; ++col) {
      Entity e;
      e.mesh = meshes[col];
      e.material = mats[row];
      e.localAABB = boxes[col];
      e.position = Vec3{ (col - 2) * xStep, 0.5f, (row - 1.5f) * zStep };
      e.scaling  = Vec3{1,1,1};
      scene_.entities.push_back(e);
    }
  }

  // Lights.
  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.4f, -1.0f, -0.3f});
  sun.color = Vec3{1.0f, 0.95f, 0.85f}; sun.intensity = 0.9f;
  scene_.lights.push_back(sun);
  Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 0};
  p.color = Vec3{1.0f, 0.6f, 0.3f}; p.intensity = 2.5f; p.radius = 12.0f;
  scene_.lights.push_back(p);

  flyCam_.setLook(Vec3{0.0f, 4.0f, 7.0f}, Vec3{0,1,0});
  scene_.camera.zFar = 100.0f;
}

} // namespace tyro
