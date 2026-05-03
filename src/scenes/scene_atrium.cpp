// Scene 9: ATRIUM — full enclosed environment exercising every feature in
// the engine. Brick walls + wood floor + marble ceiling (PBR + textures),
// 6 marble columns, central pedestal with iridescent orb, water pool,
// hologram display, explode-shader torus, teapot + spot the cow on plinths,
// 6 flickering torch point lights, a key sun streaming through a skylight,
// a stage spotlight on the orb, PCF shadows, IBL ambient + skybox.
//
// tickActiveScene() flickers the torches and pulses their marker spheres.

#include "../Demo.h"

#include <memory>

namespace tyro {

void Demo::buildSceneAtrium() {
  const float W = 24.0f, H = 8.0f, L = 24.0f;
  const float skyHalf = 3.0f; // half-width of the central skylight opening

  auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc) {
    Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
    e.position = pos; e.scaling = sc;
    scene_.entities.push_back(e);
  };

  // Per-scene material clones so each surface tiles its texture nicely.
  auto cloneMat = [&](Material* base, Vec2 uv) -> Material* {
    auto m = std::make_unique<Material>(*base);
    m->uvScale = uv;
    Material* p = m.get();
    scene_.materials.push_back(std::move(m));
    return p;
  };
  Material* matWallBrick    = cloneMat(matPbrBrick_,  Vec2{6, 2});
  Material* matFloorWood    = cloneMat(matPbrWood_,   Vec2{8, 8});
  Material* matCeilMarble   = cloneMat(matPbrMarble_, Vec2{4, 4});
  Material* matColumnMarble = cloneMat(matPbrMarble_, Vec2{1, 2});

  // ---- Floor + 4 walls (cubes scaled flat — back-face cull naturally
  // hides the outside surface so we see them from inside the room).
  put(meshCube_, cubeAABB_, matFloorWood,  Vec3{0, -0.05f, 0},      Vec3{W*0.5f, 0.05f, L*0.5f});
  put(meshCube_, cubeAABB_, matWallBrick,  Vec3{0,    H*0.5f, -L*0.5f}, Vec3{W*0.5f, H*0.5f, 0.1f}); // back
  put(meshCube_, cubeAABB_, matWallBrick,  Vec3{0,    H*0.5f,  L*0.5f}, Vec3{W*0.5f, H*0.5f, 0.1f}); // front
  put(meshCube_, cubeAABB_, matWallBrick,  Vec3{-W*0.5f, H*0.5f, 0},   Vec3{0.1f, H*0.5f, L*0.5f}); // left
  put(meshCube_, cubeAABB_, matWallBrick,  Vec3{ W*0.5f, H*0.5f, 0},   Vec3{0.1f, H*0.5f, L*0.5f}); // right

  // ---- Ceiling: 4 strips around a central skylight square. Sun + IBL
  // stream in through the hole and cast crisp shadows on the floor.
  float strip = (L*0.5f - skyHalf) * 0.5f;
  float strip2 = (W*0.5f - skyHalf) * 0.5f;
  float zMid = skyHalf + strip;
  float xMid = skyHalf + strip2;
  put(meshCube_, cubeAABB_, matCeilMarble, Vec3{0, H, -zMid}, Vec3{W*0.5f, 0.1f, strip}); // north
  put(meshCube_, cubeAABB_, matCeilMarble, Vec3{0, H,  zMid}, Vec3{W*0.5f, 0.1f, strip}); // south
  put(meshCube_, cubeAABB_, matCeilMarble, Vec3{-xMid, H, 0}, Vec3{strip2, 0.1f, skyHalf}); // west
  put(meshCube_, cubeAABB_, matCeilMarble, Vec3{ xMid, H, 0}, Vec3{strip2, 0.1f, skyHalf}); // east

  // ---- 6 columns ------------------------------------------------------
  auto column = [&](float x, float z) {
    put(meshCylinder_, cylAABB_, matColumnMarble,
        Vec3{x, H*0.5f, z}, Vec3{0.55f, H*0.5f, 0.55f});
  };
  float cx = W*0.5f - 2.5f;
  float cz = L*0.5f - 2.5f;
  column( cx,  cz); column(-cx,  cz);
  column( cx, -cz); column(-cx, -cz);
  column( cx, 0);   column(-cx, 0);

  // ---- Central altar: wood pedestal + iridescent orb ------------------
  put(meshCube_,   cubeAABB_,   matPbrWood_,    Vec3{0, 0.4f, 0}, Vec3{0.6f, 0.4f, 0.6f});
  put(meshSphere_, sphereAABB_, matIridescent_, Vec3{0, 1.35f, 0}, Vec3{0.55f, 0.55f, 0.55f});

  // ---- Side features --------------------------------------------------
  // Water pool — corner alcove. meshWater_ is a 20x20 dense plane.
  put(meshWater_, waterAABB_, matWater_, Vec3{-7.0f, 0.06f, -7.0f}, Vec3{0.18f, 1.0f, 0.18f});

  // Hologram cube on plinth (against the back wall).
  put(meshCube_, cubeAABB_, matPbrWood_,  Vec3{0, 0.3f, -8.5f}, Vec3{0.7f, 0.3f, 0.7f});
  put(meshCube_, cubeAABB_, matHologram_, Vec3{0, 1.0f, -8.5f}, Vec3{0.7f, 0.7f, 0.7f});

  // Explode-shader torus on plinth (right side).
  put(meshCube_,  cubeAABB_,  matPbrWood_, Vec3{ 8.0f, 0.3f,  0.0f}, Vec3{0.7f, 0.3f, 0.7f});
  put(meshTorus_, torusAABB_, matExplode_, Vec3{ 8.0f, 1.2f,  0.0f}, Vec3{1.0f, 1.0f, 1.0f});

  // Spot the cow display (front).
  if (meshSpot_) {
    Vec3 sc = fitScale(spotAABB_, 1.5f);
    put(meshCube_, cubeAABB_, matPbrWood_, Vec3{0, 0.3f, 8.0f}, Vec3{0.9f, 0.3f, 0.9f});
    put(meshSpot_, spotAABB_, matLitWarm_, Vec3{0, 0.9f, 8.0f}, sc);
  }

  // Teapot display (left side).
  if (meshTeapot_) {
    Vec3 sc = fitScale(teapotAABB_, 1.5f);
    put(meshCube_,   cubeAABB_,   matPbrWood_, Vec3{-8.0f, 0.3f, 0.0f}, Vec3{0.7f, 0.3f, 0.7f});
    put(meshTeapot_, teapotAABB_, matLitWarm_, Vec3{-8.0f, 0.7f, 0.0f}, sc);
  }

  // ---- Lights ---------------------------------------------------------
  // Key light: sun streaming down through the skylight, casts shadows.
  Light sun; sun.type = LightType::Directional;
  sun.direction = normalize(Vec3{-0.1f, -1.0f, 0.05f});
  sun.color = Vec3{1.0f, 0.92f, 0.78f}; sun.intensity = 1.5f;
  scene_.lights.push_back(sun);

  // 6 torches at column tops, warm and flickering. We track the first
  // entity index to drive marker spheres in tickActiveScene.
  Vec2 torchXZ[6] = {
    { cx,  cz}, {-cx,  cz}, { cx, -cz}, {-cx, -cz}, { cx, 0}, {-cx, 0},
  };
  Vec3 torchColor{1.0f, 0.55f, 0.20f};
  atriumTorchEntityStart_ = static_cast<int>(scene_.entities.size());
  for (int i = 0; i < 6; ++i) {
    Light torch; torch.type = LightType::Point;
    torch.position  = Vec3{torchXZ[i].x, H - 1.0f, torchXZ[i].y};
    torch.color     = torchColor;
    torch.intensity = 4.0f;
    torch.radius    = 8.0f;
    scene_.lights.push_back(torch);

    // Emissive marker sphere — visualises each torch.
    auto m = std::make_unique<Material>();
    m->shader   = shUnlit_;
    m->albedo   = torchColor;
    m->emissive = torchColor * 1.5f;
    Material* mp = m.get();
    scene_.materials.push_back(std::move(m));
    Entity e; e.mesh = meshSphere_; e.material = mp; e.localAABB = sphereAABB_;
    e.position = torch.position; e.scaling = Vec3{0.12f, 0.12f, 0.12f};
    scene_.entities.push_back(e);
  }

  // Stage spotlight from the ceiling, focused on the central orb. The
  // tight cone (10° inner / 16° outer) makes a clear pool of cool light
  // on the iridescent sphere — visualise the cone with `L`.
  Light stageSpot;
  stageSpot.type      = LightType::Spot;
  stageSpot.position  = Vec3{0.0f, H - 0.5f, 0.0f};
  stageSpot.direction = Vec3{0.0f, -1.0f, 0.0f};
  stageSpot.color     = Vec3{0.55f, 0.75f, 1.0f};
  stageSpot.intensity = 6.0f;
  stageSpot.radius    = 9.0f;
  stageSpot.innerDeg  = 10.0f;
  stageSpot.outerDeg  = 16.0f;
  scene_.lights.push_back(stageSpot);

  flyCam_.setLook(Vec3{0, 1.7f, 9.5f}, Vec3{0, 1.4f, 0});
  scene_.camera.zFar = 100.0f;
}

} // namespace tyro
