#pragma once
// Demo — the application class for tyro's full-feature demo executable.
//
// Owns the renderer, FBOs, IBL bake, scene container, and all 14 demo
// scenes. main.cpp keeps the engine-loop overrides (onInit/onRender/etc.)
// and the dispatch tables; each `buildSceneXxx()` is defined in its own
// `src/scenes/scene_<name>.cpp` so the per-scene geometry, material, and
// light setup lives next to the concept it teaches.
//
// Members are public for the simplest possible scene-builder access pattern.
// Demo is internal to the demo binary — there's no library API to defend,
// so the field-access vs. friend tradeoff lands on field-access.

#include <functional>
#include <memory>
#include <vector>

#include "core/Engine.h"
#include "math/Math.h"
#include "renderer/DebugDraw.h"
#include "renderer/FrameBuffer.h"
#include "renderer/GpuTimer.h"
#include "renderer/IblBaker.h"
#include "renderer/Mesh.h"
#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "renderer/ShadowMap.h"
#include "renderer/Skybox.h"
#include "renderer/TextRenderer.h"
#include "renderer/Texture.h"
#include "scene/FlyCamera.h"
#include "scene/Scene.h"

namespace tyro {

class Demo : public Application {
public:
  // ---- Engine overrides -----------------------------------------------------
  bool   onInit(Window& w) override;
  double frameRateTarget() const override;
  void   onShutdown() override;
  void   onResize(int w, int h) override;
  void   onUpdate(float dt) override;
  void   onRender(float alpha) override;

  // ---- Helpers used by main.cpp + scene_*.cpp -------------------------------
  bool   loadShaders();
  void   buildMeshesAndMaterials();
  void   buildScene(int idx);

  // Scene-info table (name + description); shared by main.cpp's overlay and
  // by buildScene() itself. Pure data lookup — no per-scene state.
  struct SceneInfo { const char* name; const char* desc; };
  static SceneInfo sceneInfo(int idx);

  // Uniform scale that fits an AABB inside a cube of side length `targetMax`.
  // Used by scene builders to normalise OBJ-loaded models like the teapot.
  static Vec3 fitScale(const AABB& box, float targetMax);

  // ---- Per-scene builders — each defined in src/scenes/scene_*.cpp ----------
  void buildSceneMaterials();
  void buildSceneLightGarden();
  void buildSceneOctreeStress();
  void buildSceneEffects();
  void buildSceneShowcase();
  void buildSceneWaterPond();
  void buildSceneGeometryLab();
  void buildSceneTextureShowcase();
  void buildScenePbrShowcase();
  void buildSceneAtrium();
  void buildSceneSpotStage();
  void buildSceneShadowTheatre();
  void buildSceneDayNight();
  void buildSceneIblFocus();

  // ---- Per-frame helpers (defined in main.cpp) ------------------------------
  void tickActiveScene(float dt);
  void handleInput(float dt);
  void updateFrameStats(float dt);
  void runFs(FrameBuffer* target, Shader* sh, std::function<void(Shader&)> setU);
  void drawShadowPreview();
  void runPostFx();
  void drawOverlay();
  const char* postFxName() const;

  // ---- State ----------------------------------------------------------------
  Window*  window_ = nullptr;
  Renderer renderer_;
  TextRenderer text_;
  DebugDraw    debug_;
  FullscreenTriangle fsTri_;
  FrameBuffer sceneFbo_;
  FrameBuffer pingFbo_, pongFbo_;
  ShadowMap shadowMap_;
  GpuTimer  gpuTimer_;
  Scene    scene_;
  FlyCamera flyCam_;

  // IBL — baked once at startup from a procedural HDR sky.
  IblBaker     iblBaker_;
  Skybox       skybox_;
  unsigned int hdrEquirect_   = 0;
  float        skyboxExposure_ = 0.6f;

  // Resource quick-refs (owned by scene_).
  Mesh* meshCube_ = nullptr; Mesh* meshSphere_ = nullptr; Mesh* meshPlane_ = nullptr;
  Mesh* meshTorus_ = nullptr; Mesh* meshCylinder_ = nullptr; Mesh* meshGround_ = nullptr;
  Mesh* meshTeapot_ = nullptr; Mesh* meshSpot_ = nullptr; Mesh* meshWater_ = nullptr;
  AABB  cubeAABB_, sphereAABB_, planeAABB_, torusAABB_, cylAABB_, groundAABB_;
  AABB  teapotAABB_, spotAABB_, waterAABB_;

  Shader* shLit_=nullptr, *shUnlit_=nullptr, *shToon_=nullptr, *shRim_=nullptr,
        * shChecker_=nullptr, *shNormalsGeo_=nullptr, *shWire_=nullptr,
        * shShadowDepth_=nullptr, *shWater_=nullptr, *shExplode_=nullptr,
        * shMarbleProc_=nullptr, *shWoodProc_=nullptr, *shBrickProc_=nullptr,
        * shHexProc_=nullptr,    *shIridescent_=nullptr, *shHologram_=nullptr,
        * shPbr_=nullptr;
  Shader* shPass_=nullptr, *shSobel_=nullptr, *shBright_=nullptr, *shBlur_=nullptr,
        * shComp_=nullptr, *shFog_=nullptr, *shSsao_=nullptr, *shChrom_=nullptr,
        * shFxaa_=nullptr, *shGodrays_=nullptr,
        * shTonemap_=nullptr, *shShadowPreview_=nullptr;

  Material* matLitWarm_=nullptr, *matLitCool_=nullptr, *matToon_=nullptr,
          * matRim_=nullptr, *matChecker_=nullptr, *matUnlit_=nullptr,
          * matEmissive_=nullptr, *matWire_=nullptr,
          * matWater_=nullptr, *matExplode_=nullptr;
  Material* matMarbleProc_=nullptr, *matWoodProc_=nullptr, *matBrickProc_=nullptr,
          * matHexProc_=nullptr,    *matIridescent_=nullptr, *matHologram_=nullptr;
  Material* matTexBrick_=nullptr,   *matTexWood_=nullptr,    *matTexMarble_=nullptr,
          * matTexHex_=nullptr,     *matTexChecker_=nullptr;
  Material* matPbrBrick_=nullptr,   *matPbrWood_=nullptr,    *matPbrMarble_=nullptr,
          * matPbrGold_=nullptr,    *matPbrCopper_=nullptr,  *matPbrPlastic_=nullptr;

  std::vector<std::unique_ptr<Texture>> textures_;
  Texture* texCheckerImg_=nullptr, *texBrickImg_=nullptr, *texWoodImg_=nullptr,
         * texMarbleImg_=nullptr,  *texNoiseImg_=nullptr, *texHexImg_=nullptr,
         * texNormalRough_=nullptr;

  // Scene-specific entity / light index trackers used by tickActiveScene().
  int waterEntityIdx_         = -1;
  int waterFloatStart_        = -1;
  int atriumTorchEntityStart_ = -1;
  int lightGardenStart_       = 0;
  int shadowTheatreFirstMover_ = -1;
  int dayNightTorchStart_      = -1;

  enum class PostFx { None, Sobel, Bloom, Fog, Ssao, Chromatic, Fxaa, Godrays, COUNT };
  PostFx postFx_ = PostFx::None;
  float  exposure_ = 1.0f;
  enum class FpsMode { Fps120, Fps60, Unlocked, COUNT };
  FpsMode fpsMode_ = FpsMode::Fps120;

  std::vector<int> visible_;
  int sceneIdx_ = 0;
  const char* sceneName_ = "?";
  const char* sceneDesc_ = "";

  bool showNormals_       = false;
  bool showWireOverlay_   = false;
  bool showDebugBounds_   = false;
  bool showDebugLights_   = false;
  bool showShadowPreview_ = false;

  // Mouse-pick state. Left-click casts a ray from the cursor and stores the
  // closest-hit entity index, which gets a magenta wire AABB on top of the
  // bounds overlay so the highlight is always visible.
  int  pickedEntity_ = -1;
  bool prevLMB_      = false;

  // 8 world-space corners of the directional light's shadow frustum, set
  // each frame alongside lightVP. Read by the debug-light overlay.
  Vec3 lightFrustumCorners_[8] = {};
  bool shadowsEnabled_  = true;
  bool iblEnabled_      = true;
  bool cullingOn_       = true;
  bool showOverlay_     = true;
  float hotReloadTimer_ = 0.0f;

  // Stats.
  float fpsAvg_ = 60.0f;
  float frameMs_ = 0.0f;
  bool  gpuMemSupported_ = false;
  int   gpuMemTotalKB_ = 0;
  int   gpuMemFreeKB_  = 0;
  int   lastDrawCalls_ = 0;
  int   lastTriangles_ = 0;
  int   visibleCount_  = 0;
  float overlayTimer_  = 0.0f;
  int   fbWidth_ = 1, fbHeight_ = 1;
  float time_ = 0.0f;

  // Edge-detected key state.
  bool prev1_=false, prev2_=false, prev3_=false, prev4_=false,
       prev5_=false, prev6_=false, prev7_=false;
  bool prevTab_=false, prevP_=false, prevN_=false, prevF_=false,
       prevT_=false,   prevJ_=false, prevK_=false, prevV_=false,
       prevB_=false,   prevL_=false, prevM_=false;
  bool prevLB_=false, prevRB_=false, prevH_=false;
};

} // namespace tyro
