// tyro demo — full feature showcase.
//
// Hotkeys:
//   [ / ]      previous / next scene
//   P          cycle post-process effect
//   N          toggle geometry-shader normals overlay
//   T          toggle wireframe overlay
//   J          toggle directional-light shadow mapping
//   K          toggle image-based lighting (IBL ambient + skybox)
//   V          cycle FPS cap (120 / 60 / unlocked)
//   F          toggle frustum culling (compare visible counts)
//   Tab        toggle mouse capture (mouse-look only when captured)
//   H          toggle stats overlay
//   1..7       override material on every entity
//                1=Phong warm  2=Phong cool  3=Toon  4=Rim  5=Checker
//                6=Unlit matcap  7=Wireframe
//   WASD       move; Q/E (or Ctrl/Space) up/down; Shift = sprint
//   ESC        quit
//
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <random>
#include <string>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include "core/Engine.h"
#include "math/Math.h"
#include "renderer/DebugDraw.h"
#include "renderer/FrameBuffer.h"
#include "renderer/GpuTimer.h"
#include "renderer/Mesh.h"
#include "renderer/Primitives.h"
#include "renderer/Renderer.h"
#include "renderer/Shader.h"
#include "renderer/ShadowMap.h"
#include "renderer/TextRenderer.h"
#include "renderer/Texture.h"
#include "renderer/ProceduralTextures.h"
#include "renderer/Cubemap.h"
#include "renderer/IblBaker.h"
#include "renderer/Skybox.h"
#include "loader/ObjLoader.h"
#include "scene/FlyCamera.h"
#include "scene/Scene.h"
#include "gl_loader.h"

using namespace tyro;

namespace {

AABB meshAABB(const std::vector<Vertex>& verts) {
  AABB b = AABB::empty();
  for (const auto& v : verts) b.expand(v.position);
  return b;
}

Mesh* makeOwnedMesh(Scene& scene, void (*gen)(std::vector<Vertex>&, std::vector<uint32_t>&),
                   AABB& outAABB) {
  std::vector<Vertex> v; std::vector<uint32_t> i;
  gen(v, i);
  outAABB = meshAABB(v);
  auto m = std::make_unique<Mesh>();
  m->upload(v, i);
  Mesh* p = m.get();
  scene.meshes.push_back(std::move(m));
  return p;
}

float frand(std::mt19937& rng, float lo, float hi) {
  std::uniform_real_distribution<float> d(lo, hi);
  return d(rng);
}

} // namespace

class Demo : public Application {
public:
  bool onInit(Window& w) override {
    window_ = &w;
    renderer_.init();

    int fbw = 0, fbh = 0;
    w.framebufferSize(fbw, fbh);
    fbWidth_  = fbw > 0 ? fbw : 1;
    fbHeight_ = fbh > 0 ? fbh : 1;
    // Scene FBO is RGBA16F so PBR + IBL + sun-disc HDR aren't clipped before
    // post-process. ping/pong stay HDR for the bloom blur path; the final
    // tonemap pass writes to the default LDR framebuffer.
    sceneFbo_.create(fbWidth_, fbHeight_, DepthMode::Texture, /*hdr*/true);
    pingFbo_ .create(fbWidth_, fbHeight_, DepthMode::None,    /*hdr*/true);
    pongFbo_ .create(fbWidth_, fbHeight_, DepthMode::None,    /*hdr*/true);

    if (!loadShaders())   return false;
    buildMeshesAndMaterials();
    if (!text_.init())    return false;
    if (!debug_.init())   return false;

    if (!shadowMap_.create(2048)) return false;
    gpuTimer_.init({"SHADOW", "SCENE", "SKY", "OVRLAY", "POSTFX", "UI"});

    // ---- Bake the IBL pipeline once at startup (procedural HDR sky) ------
    {
      // Match Scene 9's sun direction so the bright sun lines up with the
      // shadow-casting light for visual coherence.
      Vec3 sunDir = -normalize(Vec3{-0.4f, -1.0f, -0.3f});
      auto skyHdr = makeProceduralSkyHDR(1024, 512, sunDir, 80.0f);
      hdrEquirect_ = uploadEquirectHDR(skyHdr.data(), 1024, 512);
      if (!iblBaker_.bakeFromEquirect(hdrEquirect_, 1024, 512)) return false;
      if (!skybox_.init()) return false;
    }

    // Detect NVIDIA GPU memory extension by attempting a query and inspecting
    // glGetError. Cheap and self-contained.
    GLint total = 0;
    glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &total);
    if (glGetError() == GL_NO_ERROR && total > 0) {
      gpuMemSupported_ = true;
      gpuMemTotalKB_ = total;
    }

    // Capture cursor for fly-cam by default; Tab toggles.
    w.setCursorCaptured(true);

    buildScene(0);
    return true;
  }

  double frameRateTarget() const override {
    switch (fpsMode_) {
      case FpsMode::Fps120:    return 120.0;
      case FpsMode::Fps60:     return 60.0;
      case FpsMode::Unlocked:  return 0.0;
      default:                 return 0.0;
    }
  }

  void onShutdown() override {
    if (hdrEquirect_) { glDeleteTextures(1, &hdrEquirect_); hdrEquirect_ = 0; }
    iblBaker_.destroy();
    skybox_.destroy();
  }

  void onResize(int w, int h) override {
    if (w <= 0 || h <= 0) return;
    fbWidth_  = w;
    fbHeight_ = h;
    sceneFbo_.resize(w, h);
    pingFbo_ .resize(w, h);
    pongFbo_ .resize(w, h);
    scene_.camera.aspect = float(w) / float(h);
  }

  void onUpdate(float dt) override {
    time_ += dt;
    handleInput(dt);
    flyCam_.update(*window_, dt);
    flyCam_.apply(scene_.camera);
    tickActiveScene(dt);
    updateFrameStats(dt);

    // Hot-reload tick — every shader polls its source files' modtimes.
    // A failed recompile keeps the previous program (logged to stderr).
    hotReloadTimer_ += dt;
    if (hotReloadTimer_ >= 0.25f) {
      hotReloadTimer_ = 0.0f;
      for (auto& s : scene_.shaders) s->reloadIfChanged();
      text_.reloadShaderIfChanged();
      skybox_.reloadShaderIfChanged();
      debug_.reloadShaderIfChanged();
    }
  }

  void onRender(float /*alpha*/) override {
    auto& rs = renderStats();
    rs.drawCalls = 0;
    rs.triangles = 0;

    // Pump scene-time so water + explode shaders animate even when no fixed
    // updates ran this frame (e.g. when render outpaces fixed-step update).
    scene_.time = time_;

    // Reset the wireframe shader's per-frame defaults to its "material mode"
    // look — the overlay pass below may override them temporarily.
    shWire_->bind();
    shWire_->setVec3 ("uWireColor", Vec3{0.05f, 0.05f, 0.07f});
    shWire_->setVec3 ("uFillColor", Vec3{0.85f, 0.85f, 0.9f});
    shWire_->setFloat("uFillAlpha", 1.0f);
    shWire_->setFloat("uThickness", 1.5f);

    // ---- Cull -----------------------------------------------------------
    visible_.clear();
    if (cullingOn_) {
      scene_.cullVisible(visible_);
    } else {
      visible_.resize(scene_.entities.size());
      for (size_t i = 0; i < visible_.size(); ++i) visible_[i] = static_cast<int>(i);
    }
    visibleCount_ = static_cast<int>(visible_.size());

    // Compute the directional sun's view-projection if a directional light
    // exists at lights[0]. Used for both the shadow pass and the lit pass.
    Mat4 lightVP = Mat4::identity();
    bool haveSun = !scene_.lights.empty()
                && scene_.lights[0].type == LightType::Directional;
    if (haveSun) {
      Vec3 dir = normalize(scene_.lights[0].direction);
      Vec3 focus{0, 0, 0};
      Vec3 eye   = focus - dir * 30.0f;
      Vec3 upIn  = (std::abs(dir.y) > 0.99f) ? Vec3{0,0,1} : Vec3{0,1,0};
      const float boxHalf = 18.0f;
      const float zNear   = 0.1f;
      const float zFar    = 60.0f;
      lightVP = ortho(-boxHalf, boxHalf, -boxHalf, boxHalf, zNear, zFar)
              * lookAt(eye, focus, upIn);

      // Store the 8 world-space corners of the shadow frustum so the debug
      // overlay can wire-draw it without needing a general Mat4 inverse.
      // Order: near face BL/BR/TR/TL, then far face same winding.
      Vec3 fwd   = normalize(focus - eye);
      Vec3 right = normalize(cross(fwd, upIn));
      Vec3 upWS  = cross(right, fwd);
      auto corner = [&](int xs, int ys, float z) {
        return eye + fwd * z
                   + right * (boxHalf * float(xs))
                   + upWS  * (boxHalf * float(ys));
      };
      lightFrustumCorners_[0] = corner(-1, -1, zNear);
      lightFrustumCorners_[1] = corner(+1, -1, zNear);
      lightFrustumCorners_[2] = corner(+1, +1, zNear);
      lightFrustumCorners_[3] = corner(-1, +1, zNear);
      lightFrustumCorners_[4] = corner(-1, -1, zFar);
      lightFrustumCorners_[5] = corner(+1, -1, zFar);
      lightFrustumCorners_[6] = corner(+1, +1, zFar);
      lightFrustumCorners_[7] = corner(-1, +1, zFar);
    }

    // ---- Pass 0: shadow map (depth-only) -------------------------------
    gpuTimer_.begin(0);
    if (haveSun && shadowsEnabled_) {
      // Make sure the lit pass doesn't try to bind the shadow texture as
      // input while we're writing into it.
      scene_.shadowEnabled = false;
      scene_.shadowMapTex  = 0;

      shadowMap_.bind();
      glClear(GL_DEPTH_BUFFER_BIT);
      glEnable(GL_DEPTH_TEST);
      glDepthFunc(GL_LESS);
      glEnable(GL_POLYGON_OFFSET_FILL);
      glPolygonOffset(2.0f, 4.0f);

      shShadowDepth_->bind();
      shShadowDepth_->setMat4("uLightVP", lightVP);
      for (const auto& e : scene_.entities) {
        if (!e.mesh) continue;
        shShadowDepth_->setMat4("uModel", e.modelMatrix());
        e.mesh->draw();
      }

      glDisable(GL_POLYGON_OFFSET_FILL);
    }
    gpuTimer_.end(0);

    // Now prime Scene with shadow data so lit shaders sample the map.
    scene_.lightVP       = lightVP;
    scene_.shadowEnabled = haveSun && shadowsEnabled_;
    scene_.shadowMapTex  = shadowsEnabled_ ? shadowMap_.depthTexture() : 0;

    // Prime IBL inputs every frame; Scene::uploadSceneUniforms binds them
    // to TEX5/6/7 the first time each shader is touched.
    scene_.iblEnabled         = iblEnabled_;
    scene_.irradianceCubemap  = iblBaker_.irradianceMap().handle();
    scene_.prefilterCubemap   = iblBaker_.prefilterMap().handle();
    scene_.brdfLut            = iblBaker_.brdfLut();
    scene_.prefilterMipLevels = iblBaker_.prefilterMipLevels();

    // ---- Pass 1: scene → sceneFbo (color + depth) ----------------------
    gpuTimer_.begin(1);
    sceneFbo_.bind();
    renderer_.enableDepth(true);
    renderer_.enableCulling(true);
    renderer_.clear(Vec3{0.06f, 0.07f, 0.09f});

    scene_.render(visible_);
    gpuTimer_.end(1);

    // ---- Pass 1.5: skybox -----------------------------------------------
    gpuTimer_.begin(2);
    if (iblEnabled_) {
      // Draw with depth=LEQUAL so the cube fills only un-rasterised pixels.
      // Disable depth writes so the box never occludes geometry written
      // beforehand, and re-enable culling-friendly state on the way out.
      glDepthFunc(GL_LEQUAL);
      glDepthMask(GL_FALSE);
      glDisable(GL_CULL_FACE);
      skybox_.render(iblBaker_.environmentMap(),
                     scene_.camera.view(), scene_.camera.projection(),
                     skyboxExposure_);
      glEnable(GL_CULL_FACE);
      glDepthMask(GL_TRUE);
      glDepthFunc(GL_LESS);
    }
    gpuTimer_.end(2);

    // ---- Pass 1b/1c: optional debug overlays ---------------------------
    gpuTimer_.begin(3);
    if (showNormals_) {
      shNormalsGeo_->bind();
      shNormalsGeo_->setMat4 ("uViewProj", scene_.camera.viewProj());
      shNormalsGeo_->setFloat("uLength",   0.18f);
      glLineWidth(1.5f);
      scene_.render(visible_, shNormalsGeo_);
    }
    if (showWireOverlay_) {
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDepthFunc(GL_LEQUAL);
      shWire_->bind();
      shWire_->setVec3 ("uWireColor", Vec3{0.2f, 1.0f, 1.0f});
      shWire_->setVec3 ("uFillColor", Vec3{0,0,0});
      shWire_->setFloat("uFillAlpha", 0.0f);
      shWire_->setFloat("uThickness", 1.5f);
      scene_.render(visible_, shWire_);
      glDisable(GL_BLEND);
      glDepthFunc(GL_LESS);
    }
    if (showDebugBounds_ || showDebugLights_) {
      if (showDebugBounds_) {
        // Green: each visible entity's world AABB. Orange: every octree node.
        // Press F to toggle culling and watch the green boxes blink in/out.
        for (int i : visible_) {
          debug_.aabb(scene_.entities[i].worldAABB(), Vec3{0.2f, 1.0f, 0.4f});
        }
        std::vector<AABB> nodeBounds;
        scene_.octreeNodeBounds(nodeBounds);
        for (const AABB& b : nodeBounds) {
          debug_.aabb(b, Vec3{1.0f, 0.6f, 0.2f});
        }
      }
      if (showDebugLights_) {
        // Point lights: wire sphere at the position, tinted by light colour
        // so a red point light gets a red sphere. Directional sun: yellow
        // wire frustum showing the volume the shadow map covers.
        for (const auto& L : scene_.lights) {
          if (L.type == LightType::Point) {
            debug_.sphere(L.position, L.radius, L.color);
          }
        }
        if (haveSun) {
          debug_.wireFrustum(lightFrustumCorners_, Vec3{1.0f, 0.95f, 0.4f});
        }
      }
      glLineWidth(1.5f);
      debug_.flush(scene_.camera.viewProj());
    }
    gpuTimer_.end(3);

    // ---- Pass 2: post-process chain → default ---------------------------
    gpuTimer_.begin(4);
    runPostFx();
    gpuTimer_.end(4);

    // ---- Pass 3: text overlay onto default ------------------------------
    gpuTimer_.begin(5);
    if (showOverlay_) drawOverlay();
    gpuTimer_.end(5);

    gpuTimer_.endFrame();

    lastDrawCalls_ = renderStats().drawCalls;
    lastTriangles_ = renderStats().triangles;
  }

private:
  // ===== Resource loading ==================================================
  bool loadShaders() {
    auto add = [&](const char* v, const char* f, const char* g = nullptr) -> Shader* {
      auto s = std::make_unique<Shader>();
      if (!s->loadFromFiles(v, f, g)) return nullptr;
      Shader* ptr = s.get();
      scene_.shaders.push_back(std::move(s));
      return ptr;
    };
    shLit_         = add("shaders/phong_lit.vert", "shaders/phong_lit.frag"); if (!shLit_) return false;
    shUnlit_       = add("shaders/unlit.vert",     "shaders/unlit.frag");     if (!shUnlit_) return false;
    shToon_        = add("shaders/toon.vert",      "shaders/toon.frag");      if (!shToon_) return false;
    shRim_         = add("shaders/rim.vert",       "shaders/rim.frag");       if (!shRim_) return false;
    shChecker_     = add("shaders/checker.vert",   "shaders/checker.frag");   if (!shChecker_) return false;
    shNormalsGeo_  = add("shaders/normals.vert",   "shaders/normals.frag",
                                                   "shaders/normals.geom");   if (!shNormalsGeo_) return false;
    shWire_        = add("shaders/wireframe.vert", "shaders/wireframe.frag",
                                                   "shaders/wireframe.geom"); if (!shWire_) return false;
    shShadowDepth_ = add("shaders/shadow_depth.vert", "shaders/shadow_depth.frag");
    if (!shShadowDepth_) return false;
    shWater_       = add("shaders/water.vert",   "shaders/water.frag");   if (!shWater_) return false;
    shExplode_     = add("shaders/explode.vert", "shaders/explode.frag",
                                                  "shaders/explode.geom"); if (!shExplode_) return false;
    shMarbleProc_  = add("shaders/phong_lit.vert", "shaders/marble.frag");     if (!shMarbleProc_) return false;
    shWoodProc_    = add("shaders/phong_lit.vert", "shaders/wood.frag");       if (!shWoodProc_) return false;
    shBrickProc_   = add("shaders/phong_lit.vert", "shaders/brick.frag");      if (!shBrickProc_) return false;
    shHexProc_     = add("shaders/phong_lit.vert", "shaders/hex.frag");        if (!shHexProc_) return false;
    shIridescent_  = add("shaders/phong_lit.vert", "shaders/iridescent.frag"); if (!shIridescent_) return false;
    shHologram_    = add("shaders/phong_lit.vert", "shaders/hologram.frag");   if (!shHologram_) return false;
    shPbr_         = add("shaders/pbr.vert",       "shaders/pbr.frag");        if (!shPbr_) return false;

    shTonemap_= add("shaders/blit.vert", "shaders/post_tonemap.frag");    if (!shTonemap_) return false;
    shPass_   = add("shaders/blit.vert", "shaders/post_passthrough.frag"); if (!shPass_) return false;
    shSobel_  = add("shaders/blit.vert", "shaders/post_sobel.frag");      if (!shSobel_) return false;
    shBright_ = add("shaders/blit.vert", "shaders/post_bright.frag");     if (!shBright_) return false;
    shBlur_   = add("shaders/blit.vert", "shaders/post_blur.frag");       if (!shBlur_) return false;
    shComp_   = add("shaders/blit.vert", "shaders/post_composite.frag");  if (!shComp_) return false;
    shFog_    = add("shaders/blit.vert", "shaders/post_fog.frag");        if (!shFog_) return false;
    shSsao_   = add("shaders/blit.vert", "shaders/post_ssao.frag");       if (!shSsao_) return false;
    shChrom_  = add("shaders/blit.vert", "shaders/post_chromatic.frag");  if (!shChrom_) return false;
    return true;
  }

  void buildMeshesAndMaterials() {
    AABB a;
    meshCube_     = makeOwnedMesh(scene_, +[](std::vector<Vertex>& v, std::vector<uint32_t>& i){ makeCube(v, i); },     cubeAABB_);
    meshSphere_   = makeOwnedMesh(scene_, +[](std::vector<Vertex>& v, std::vector<uint32_t>& i){ makeSphere(v, i); },   sphereAABB_);
    meshPlane_    = makeOwnedMesh(scene_, +[](std::vector<Vertex>& v, std::vector<uint32_t>& i){ makePlane(v, i, 1.0f, 1); }, planeAABB_);
    meshTorus_    = makeOwnedMesh(scene_, +[](std::vector<Vertex>& v, std::vector<uint32_t>& i){ makeTorus(v, i); },    torusAABB_);
    meshCylinder_ = makeOwnedMesh(scene_, +[](std::vector<Vertex>& v, std::vector<uint32_t>& i){ makeCylinder(v, i); }, cylAABB_);
    // A large ground plane mesh shared by scenes that need it.
    {
      std::vector<Vertex> v; std::vector<uint32_t> i;
      makePlane(v, i, 30.0f, 8);
      groundAABB_ = meshAABB(v);
      auto m = std::make_unique<Mesh>(); m->upload(v, i);
      meshGround_ = m.get();
      scene_.meshes.push_back(std::move(m));
    }

    // Densely-subdivided plane for water vertex displacement.
    {
      std::vector<Vertex> v; std::vector<uint32_t> i;
      makePlane(v, i, 20.0f, 64);
      AABB ab = meshAABB(v);
      ab.min.y -= 0.5f; ab.max.y += 0.5f; // include displacement amplitude
      waterAABB_ = ab;
      auto m = std::make_unique<Mesh>(); m->upload(v, i);
      meshWater_ = m.get();
      scene_.meshes.push_back(std::move(m));
    }

    // Real OBJ models — Newell teapot and Keenan Crane's "Spot".
    auto loadObjMesh = [&](const char* path, AABB& outAABB) -> Mesh* {
      std::vector<Vertex> v; std::vector<uint32_t> i;
      if (!loadObj(path, v, i)) return nullptr;
      outAABB = meshAABB(v);
      auto m = std::make_unique<Mesh>(); m->upload(v, i);
      Mesh* p = m.get();
      scene_.meshes.push_back(std::move(m));
      std::fprintf(stderr, "[demo] %s: %zu verts, %zu indices\n", path, v.size(), i.size());
      return p;
    };
    meshTeapot_ = loadObjMesh("assets/teapot.obj", teapotAABB_);
    meshSpot_   = loadObjMesh("assets/spot.obj",   spotAABB_);

    auto addMat = [&](Shader* sh, Vec3 albedo, float shin, Vec3 em = Vec3{0,0,0}) -> Material* {
      auto m = std::make_unique<Material>();
      m->shader = sh; m->albedo = albedo; m->shininess = shin; m->emissive = em;
      Material* ptr = m.get();
      scene_.materials.push_back(std::move(m));
      return ptr;
    };
    matLitWarm_  = addMat(shLit_,     Vec3{0.85f, 0.45f, 0.25f}, 64.0f);
    matLitCool_  = addMat(shLit_,     Vec3{0.30f, 0.55f, 0.85f}, 16.0f);
    matToon_     = addMat(shToon_,    Vec3{0.9f,  0.8f,  0.4f},  1.0f);
    matRim_      = addMat(shRim_,     Vec3{0.2f,  0.8f,  1.0f},  1.0f, Vec3{0.05f,0.4f,0.7f});
    matChecker_  = addMat(shChecker_, Vec3{0.7f,  0.7f,  0.7f},  20.0f);
    matUnlit_    = addMat(shUnlit_,   Vec3{0.9f,  0.9f,  0.9f},  1.0f);
    matEmissive_ = addMat(shUnlit_,   Vec3{1.0f,  1.0f,  1.0f},  1.0f, Vec3{1.0f, 0.6f, 0.2f});
    matWire_     = addMat(shWire_,    Vec3{0.85f, 0.85f, 0.9f},  1.0f);
    matWater_    = addMat(shWater_,   Vec3{0.10f, 0.40f, 0.55f}, 1.0f);
    matExplode_  = addMat(shExplode_, Vec3{0.95f, 0.55f, 0.20f}, 32.0f);

    // ---- Procedural-pattern materials (no texture; computed in fragment) ----
    matMarbleProc_  = addMat(shMarbleProc_, Vec3{0.85f, 0.82f, 0.92f}, 80.0f);
    matWoodProc_    = addMat(shWoodProc_,   Vec3{0.55f, 0.34f, 0.15f},  8.0f);
    matBrickProc_   = addMat(shBrickProc_,  Vec3{0.70f, 0.32f, 0.22f},  4.0f);
    matHexProc_     = addMat(shHexProc_,    Vec3{0.40f, 0.90f, 0.70f}, 20.0f);
    matIridescent_  = addMat(shIridescent_, Vec3{0.95f, 0.95f, 0.98f},  1.0f);
    matHologram_    = addMat(shHologram_,   Vec3{0.30f, 0.95f, 1.00f},  1.0f);

    // ---- Generate procedural textures + bind to texture-aware materials ----
    auto bakeTex = [&](std::vector<std::uint8_t>&& pixels, int size) -> Texture* {
      auto t = std::make_unique<Texture>();
      t->loadFromMemory(size, size, 4, pixels.data());
      Texture* p = t.get();
      textures_.push_back(std::move(t));
      return p;
    };
    const int kTexSize = 256;
    texCheckerImg_ = bakeTex(makeCheckerTex(kTexSize), kTexSize);
    texBrickImg_   = bakeTex(makeBrickTex  (kTexSize), kTexSize);
    texWoodImg_    = bakeTex(makeWoodTex   (kTexSize), kTexSize);
    texMarbleImg_  = bakeTex(makeMarbleTex (kTexSize), kTexSize);
    texNoiseImg_   = bakeTex(makeNoiseTex  (kTexSize), kTexSize);
    texHexImg_     = bakeTex(makeHexTex    (kTexSize), kTexSize);
    texNormalRough_= bakeTex(makeRoughNormalMap(kTexSize, 8.0f), kTexSize);
    std::fprintf(stderr, "[demo] generated %zu procedural textures (%dx%d each)\n",
                 textures_.size(), kTexSize, kTexSize);

    // ---- Texture-mapped materials (use the texture-aware Phong shader) ----
    auto addTexMat = [&](Texture* tex, Vec3 tint, float shin, Vec2 uvScale = {2,2}) {
      auto m = std::make_unique<Material>();
      m->shader = shLit_;
      m->albedo = tint;
      m->shininess = shin;
      m->albedoTex = tex;
      m->uvScale = uvScale;
      Material* p = m.get();
      scene_.materials.push_back(std::move(m));
      return p;
    };
    matTexBrick_   = addTexMat(texBrickImg_,  Vec3{1,1,1},  8.0f, Vec2{2,2});
    matTexWood_    = addTexMat(texWoodImg_,   Vec3{1,1,1},  8.0f, Vec2{2,2});
    matTexMarble_  = addTexMat(texMarbleImg_, Vec3{1,1,1}, 80.0f, Vec2{1,1});
    matTexHex_     = addTexMat(texHexImg_,    Vec3{1,1,1}, 20.0f, Vec2{2,2});
    matTexChecker_ = addTexMat(texCheckerImg_,Vec3{1,1,1}, 16.0f, Vec2{4,4});

    // ---- PBR materials -------------------------------------------------
    auto addPbrMat = [&](Vec3 albedo, float metallic, float roughness,
                         Texture* alb = nullptr, Texture* nrm = nullptr,
                         Vec2 uvScale = {1,1}) {
      auto m = std::make_unique<Material>();
      m->shader    = shPbr_;
      m->albedo    = albedo;
      m->metallic  = metallic;
      m->roughness = roughness;
      m->albedoTex = alb;
      m->normalTex = nrm;
      m->uvScale   = uvScale;
      Material* p = m.get();
      scene_.materials.push_back(std::move(m));
      return p;
    };
    // Textured PBR materials — albedo from procedural texture + shared rough normal map.
    matPbrBrick_  = addPbrMat(Vec3{1,1,1},        0.0f, 0.85f, texBrickImg_,  texNormalRough_, Vec2{2,2});
    matPbrWood_   = addPbrMat(Vec3{1,1,1},        0.0f, 0.55f, texWoodImg_,   texNormalRough_, Vec2{2,2});
    matPbrMarble_ = addPbrMat(Vec3{1,1,1},        0.0f, 0.10f, texMarbleImg_, texNormalRough_, Vec2{1,1});

    // Uniform-color PBR materials — for the metallic / roughness sweeps below.
    matPbrGold_   = addPbrMat(Vec3{1.00f, 0.78f, 0.34f}, 1.0f, 0.20f, nullptr, texNormalRough_, Vec2{2,2});
    matPbrCopper_ = addPbrMat(Vec3{0.95f, 0.64f, 0.54f}, 1.0f, 0.35f, nullptr, texNormalRough_, Vec2{2,2});
    matPbrPlastic_= addPbrMat(Vec3{0.85f, 0.20f, 0.20f}, 0.0f, 0.40f, nullptr, texNormalRough_, Vec2{2,2});

    // Roughness sweep (metallic = 1) — generated programmatically in Scene 9.
    // We don't store these as members; they're created on scene-build.
  }

  // ===== Scene cycling =====================================================
  struct SceneInfo {
    const char* name;
    const char* desc;
  };
  static SceneInfo sceneInfo(int idx) {
    switch (idx) {
      case 0: return {"MATERIALS",     "5 mesh types x 4 materials (phong, toon, rim)"};
      case 1: return {"LIGHT GARDEN",  "6 colored point lights orbit a checker plane"};
      case 2: return {"OCTREE STRESS", "512 entities + frustum cull via static octree"};
      case 3: return {"EFFECTS",       "hero meshes with strong contrast for post-FX"};
      case 4: return {"SHOWCASE BAY",  "teapot + spot the cow, shadowed warm/cool light"};
      case 5: return {"WATER POND",    "sin-sum vertex displacement + Fresnel water"};
      case 6: return {"GEOMETRY LAB",  "explode geometry shader on teapot/cow/torus"};
      case 7: return {"TEXTURE LAB",   "GPU textures vs procedural patterns vs exotic"};
      case 8: return {"PBR LAB",       "metallic + roughness sweeps, Cook-Torrance + IBL"};
      case 9: return {"ATRIUM",        "enclosed hall: every feature in one environment"};
    }
    return {"?", ""};
  }

  void buildScene(int idx) {
    scene_.clearActiveScene();
    constexpr int kSceneCount = 10;
    sceneIdx_ = ((idx % kSceneCount) + kSceneCount) % kSceneCount;
    switch (sceneIdx_) {
      case 0: buildSceneMaterials();        break;
      case 1: buildSceneLightGarden();      break;
      case 2: buildSceneOctreeStress();     break;
      case 3: buildSceneEffects();          break;
      case 4: buildSceneShowcase();         break;
      case 5: buildSceneWaterPond();        break;
      case 6: buildSceneGeometryLab();      break;
      case 7: buildSceneTextureShowcase();  break;
      case 8: buildScenePbrShowcase();      break;
      case 9: buildSceneAtrium();           break;
    }
    SceneInfo info = sceneInfo(sceneIdx_);
    sceneName_ = info.name;
    sceneDesc_ = info.desc;
    scene_.rebuildOctree();
  }

  // Scene 10: a full enclosed environment that exercises every feature in the
  // engine — brick walls + wood floor + marble ceiling (PBR + textures), 6
  // marble columns, central pedestal with iridescent orb, water pool, hologram
  // display, explode-shader torus, teapot + spot the cow on display plinths,
  // 6 flickering torch point lights, a key sun streaming through a skylight,
  // PCF shadows, IBL ambient + skybox.
  void buildSceneAtrium() {
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

    flyCam_.setLook(Vec3{0, 1.7f, 9.5f}, Vec3{0, 1.4f, 0});
    scene_.camera.zFar = 100.0f;
  }

  void buildScenePbrShowcase() {
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

  void buildSceneTextureShowcase() {
    // Ground using the procedurally generated checker texture.
    Entity g; g.mesh = meshGround_; g.material = matTexChecker_;
    g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
    scene_.entities.push_back(g);

    // 3x3 grid of spheres demonstrating textured + procedural-pattern shaders.
    Material* row[3][3] = {
      { matTexBrick_,    matTexWood_,     matTexMarble_   }, // back: GPU textures
      { matMarbleProc_,  matWoodProc_,    matBrickProc_   }, // mid : procedural patterns
      { matHexProc_,     matIridescent_,  matHologram_    }, // front: exotic
    };
    const float spacing = 2.0f;
    for (int r = 0; r < 3; ++r) {
      for (int c = 0; c < 3; ++c) {
        Entity e;
        e.mesh = meshSphere_;
        e.localAABB = sphereAABB_;
        e.material = row[r][c];
        e.position = Vec3{ (c - 1) * spacing, 0.9f, (r - 1) * spacing };
        scene_.entities.push_back(e);
      }
    }

    // Lights — moderate sun + warm point light overhead.
    Light sun; sun.type = LightType::Directional;
    sun.direction = normalize(Vec3{-0.4f, -1.0f, -0.3f});
    sun.color = Vec3{1.0f, 0.96f, 0.88f}; sun.intensity = 0.95f;
    scene_.lights.push_back(sun);
    Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 0};
    p.color = Vec3{1.0f, 0.6f, 0.3f}; p.intensity = 2.2f; p.radius = 12.0f;
    scene_.lights.push_back(p);

    flyCam_.setLook(Vec3{0.0f, 2.5f, 5.5f}, Vec3{0, 1.0f, 0});
    scene_.camera.zFar = 100.0f;
  }

  // Pick a uniform scale for an arbitrary OBJ model so its largest dimension
  // becomes `targetMax`. Keeps mixed-source meshes sized consistently.
  static Vec3 fitScale(const AABB& box, float targetMax) {
    Vec3 sz = box.size();
    float m = std::max(std::max(sz.x, sz.y), sz.z);
    float s = m > 1e-4f ? targetMax / m : 1.0f;
    return Vec3{s, s, s};
  }

  void buildSceneShowcase() {
    Entity g; g.mesh = meshGround_; g.material = matChecker_;
    g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
    scene_.entities.push_back(g);

    auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc) {
      Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
      e.position = pos; e.scaling = sc;
      scene_.entities.push_back(e);
    };
    Vec3 teapotScale = meshTeapot_ ? fitScale(teapotAABB_, 2.5f) : Vec3{1,1,1};
    Vec3 spotScale   = meshSpot_   ? fitScale(spotAABB_,   1.6f) : Vec3{1,1,1};

    if (meshTeapot_) put(meshTeapot_, teapotAABB_, matLitWarm_, Vec3{-1.5f, 0.0f, 0.0f}, teapotScale);
    if (meshSpot_)   put(meshSpot_,   spotAABB_,   matLitCool_, Vec3{ 1.8f, 0.5f, 0.0f}, spotScale);
    put(meshSphere_, sphereAABB_, matRim_,     Vec3{ 0.0f, 0.5f,  2.5f}, Vec3{1,1,1});
    put(meshTorus_,  torusAABB_,  matToon_,    Vec3{-2.5f, 0.5f,  2.5f}, Vec3{1.4f,1.4f,1.4f});
    put(meshCube_,   cubeAABB_,   matRim_,     Vec3{ 2.5f, 0.5f, -2.5f}, Vec3{1,1,1});

    Light sun; sun.type = LightType::Directional;
    sun.direction = normalize(Vec3{-0.4f, -1.0f, -0.3f});
    sun.color = Vec3{1.0f, 0.95f, 0.85f}; sun.intensity = 0.9f;
    scene_.lights.push_back(sun);
    Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 0};
    p.color = Vec3{1.0f, 0.6f, 0.3f}; p.intensity = 2.0f; p.radius = 10.0f;
    scene_.lights.push_back(p);

    flyCam_.setLook(Vec3{0.0f, 2.5f, 6.0f}, Vec3{0,0.5f,0});
    scene_.camera.zFar = 100.0f;
  }

  void buildSceneWaterPond() {
    // Water — large displaced plane, water material.
    Entity w; w.mesh = meshWater_; w.material = matWater_;
    w.position = Vec3{0, 0, 0}; w.localAABB = waterAABB_;
    waterEntityIdx_ = static_cast<int>(scene_.entities.size());
    scene_.entities.push_back(w);

    auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc) {
      Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
      e.position = pos; e.scaling = sc;
      scene_.entities.push_back(e);
    };

    Vec3 teapotScale = meshTeapot_ ? fitScale(teapotAABB_, 1.8f) : Vec3{1,1,1};
    Vec3 spotScale   = meshSpot_   ? fitScale(spotAABB_,   1.2f) : Vec3{1,1,1};

    waterFloatStart_ = static_cast<int>(scene_.entities.size());
    if (meshTeapot_) put(meshTeapot_, teapotAABB_, matLitWarm_, Vec3{-1.0f, 0.6f,  0.0f}, teapotScale);
    if (meshSpot_)   put(meshSpot_,   spotAABB_,   matLitCool_, Vec3{ 1.5f, 0.5f, -1.0f}, spotScale);
    // Stones around the pond edge — small dark cubes.
    for (int i = 0; i < 6; ++i) {
      float a = (float(i) / 6.0f) * 2.0f * kPi;
      Vec3 pos{std::cos(a) * 7.0f, -0.1f, std::sin(a) * 7.0f};
      Vec3 sc{0.6f + (i*0.07f), 0.3f, 0.5f};
      put(meshCube_, cubeAABB_, matLitWarm_, pos, sc);
    }

    Light sun; sun.type = LightType::Directional;
    sun.direction = normalize(Vec3{-0.3f, -0.8f, -0.5f});
    sun.color = Vec3{1.0f, 0.92f, 0.78f}; sun.intensity = 1.0f;
    scene_.lights.push_back(sun);

    flyCam_.setLook(Vec3{ 0.0f, 1.5f, 7.0f}, Vec3{0, 0.3f, 0});
    scene_.camera.zFar = 120.0f;
  }

  void buildSceneGeometryLab() {
    Entity g; g.mesh = meshGround_; g.material = matChecker_;
    g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
    scene_.entities.push_back(g);

    auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc) {
      Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
      e.position = pos; e.scaling = sc;
      scene_.entities.push_back(e);
    };
    // Three "lab specimens" using the explode geom shader — pulses outward
    // along each face's normal so you can see the underlying tessellation.
    Vec3 teapotScale = meshTeapot_ ? fitScale(teapotAABB_, 2.5f) : Vec3{1,1,1};
    Vec3 spotScale   = meshSpot_   ? fitScale(spotAABB_,   1.8f) : Vec3{1,1,1};

    if (meshTeapot_) put(meshTeapot_, teapotAABB_, matExplode_, Vec3{-2.5f, 0.6f, 0.0f}, teapotScale);
    if (meshSpot_)   put(meshSpot_,   spotAABB_,   matExplode_, Vec3{ 0.0f, 0.7f, 0.0f}, spotScale);
    put(meshTorus_,  torusAABB_,  matExplode_, Vec3{ 2.5f, 0.7f, 0.0f}, Vec3{1.6f,1.6f,1.6f});

    Light sun; sun.type = LightType::Directional;
    sun.direction = normalize(Vec3{-0.4f, -1.0f, -0.3f});
    sun.color = Vec3{1,1,1}; sun.intensity = 0.9f;
    scene_.lights.push_back(sun);
    Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 2};
    p.color = Vec3{0.4f, 0.9f, 1.0f}; p.intensity = 2.5f; p.radius = 12.0f;
    scene_.lights.push_back(p);

    // Set magnitude on the explode shader. Stays for the duration of this scene.
    shExplode_->bind();
    shExplode_->setFloat("uMagnitude", 0.25f);

    flyCam_.setLook(Vec3{0.0f, 2.0f, 5.5f}, Vec3{0, 0.5f, 0});
    scene_.camera.zFar = 100.0f;
  }

  void buildSceneMaterials() {
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

  void buildSceneLightGarden() {
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

  void buildSceneOctreeStress() {
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

  void buildSceneEffects() {
    Entity g; g.mesh = meshGround_; g.material = matChecker_;
    g.position = Vec3{0,-0.5f,0}; g.localAABB = groundAABB_;
    scene_.entities.push_back(g);

    auto put = [&](Mesh* m, AABB ab, Material* mat, Vec3 pos, Vec3 sc = Vec3{1,1,1}) {
      Entity e; e.mesh = m; e.material = mat; e.localAABB = ab;
      e.position = pos; e.scaling = sc;
      scene_.entities.push_back(e);
    };
    put(meshTorus_,    torusAABB_,  matRim_,      Vec3{-2.0f, 1.0f, 0.0f}, Vec3{2,2,2});
    put(meshSphere_,   sphereAABB_, matLitCool_,  Vec3{ 0.0f, 0.7f, 0.0f}, Vec3{1.4f,1.4f,1.4f});
    put(meshCube_,     cubeAABB_,   matLitWarm_,  Vec3{ 2.5f, 0.5f, 0.0f});
    put(meshSphere_,   sphereAABB_, matEmissive_, Vec3{-1.0f, 2.5f, -2.0f}, Vec3{0.5f,0.5f,0.5f});
    put(meshSphere_,   sphereAABB_, matEmissive_, Vec3{ 1.5f, 2.0f,  2.5f}, Vec3{0.4f,0.4f,0.4f});

    Light sun; sun.type = LightType::Directional;
    sun.direction = normalize(Vec3{-0.3f, -1.0f, -0.4f});
    sun.color = Vec3{0.9f, 0.85f, 0.7f}; sun.intensity = 0.6f;
    scene_.lights.push_back(sun);
    Light p; p.type = LightType::Point; p.position = Vec3{0, 3, 0};
    p.color = Vec3{1.0f, 0.5f, 0.2f}; p.intensity = 4.0f; p.radius = 8.0f;
    scene_.lights.push_back(p);

    flyCam_.setLook(Vec3{ 0.0f, 2.0f, 6.5f}, Vec3{0,1,0});
    scene_.camera.zFar = 100.0f;
  }

  // Per-scene update (animations).
  void tickActiveScene(float dt) {
    (void)dt;
    if (sceneIdx_ == 1) {
      // Animate the 6 point lights and their marker spheres.
      int firstLightIdx = 0; // sun is appended last; the 6 colored lights are first
      for (int i = 0; i < 6 && firstLightIdx + i < (int)scene_.lights.size(); ++i) {
        float t = time_ * (0.4f + i * 0.07f) + i * 1.05f;
        Vec3 p{ std::cos(t) * (3.5f + 0.4f * i),
                1.5f + std::sin(time_ * 0.6f + i) * 0.7f,
                std::sin(t) * (3.5f + 0.4f * i) };
        scene_.lights[firstLightIdx + i].position = p;
        // Update the corresponding marker entity's position.
        int eidx = lightGardenStart_ + i;
        if (eidx < (int)scene_.entities.size())
          scene_.entities[eidx].position = p;
      }
    } else if (sceneIdx_ == 2 || sceneIdx_ == 3) {
      // Slowly drift the point light in stress / effects scenes.
      if (scene_.lights.size() >= 2) {
        scene_.lights[1].position = Vec3{
          std::cos(time_ * 0.7f) * 5.0f,
          3.0f,
          std::sin(time_ * 0.7f) * 5.0f
        };
      }
    } else if (sceneIdx_ == 0) {
      // Move the single point light for the materials scene.
      if (scene_.lights.size() >= 2) {
        scene_.lights[1].position = Vec3{
          std::cos(time_ * 0.5f) * 4.0f,
          2.5f + std::sin(time_) * 0.5f,
          std::sin(time_ * 0.5f) * 4.0f
        };
      }
    } else if (sceneIdx_ == 9) {
      // Atrium — flicker the 6 torch lights and pulse their marker spheres.
      // Sun is at lights[0]; torches are lights[1..6].
      const int firstTorch = 1;
      for (int i = 0; i < 6 && firstTorch + i < (int)scene_.lights.size(); ++i) {
        float n = std::sin(time_ * 7.3f + i * 1.7f) * 0.5f
                + std::sin(time_ * 13.1f + i * 0.9f) * 0.3f;
        scene_.lights[firstTorch + i].intensity = 4.0f + n * 0.6f;
        // Subtle marker-sphere pulse — tracks light intensity.
        if (atriumTorchEntityStart_ >= 0) {
          int eidx = atriumTorchEntityStart_ + i;
          if (eidx < (int)scene_.entities.size()) {
            float s = 0.12f + n * 0.012f;
            scene_.entities[eidx].scaling = Vec3{s, s, s};
          }
        }
      }
    } else if (sceneIdx_ == 5) {
      // Water Pond — bob the floating teapot and cow on the water surface.
      if (waterFloatStart_ >= 0) {
        for (int i = 0; i < 2; ++i) {
          int eidx = waterFloatStart_ + i;
          if (eidx < (int)scene_.entities.size()) {
            float bob = std::sin(time_ * 1.4f + i * 1.8f) * 0.10f;
            float yaw = std::sin(time_ * 0.5f + i * 0.7f) * 0.15f;
            scene_.entities[eidx].position.y = 0.55f + i * 0.05f + bob;
            scene_.entities[eidx].rotation = Quat::fromAxisAngle(Vec3{0,1,0}, yaw);
          }
        }
      }
    }
  }

  // ===== Input =============================================================
  void handleInput(float /*dt*/) {
    if (!window_) return;
    if (window_->keyPressed(GLFW_KEY_ESCAPE))
      glfwSetWindowShouldClose(window_->handle(), 1);

    auto edge = [&](int key, bool& prev, std::function<void()> action){
      bool now = window_->keyPressed(key);
      if (now && !prev) action();
      prev = now;
    };

    edge(GLFW_KEY_TAB,  prevTab_, [&]{ window_->setCursorCaptured(!window_->cursorCaptured()); });
    edge(GLFW_KEY_LEFT_BRACKET,  prevLB_, [&]{ buildScene(sceneIdx_ - 1); });
    edge(GLFW_KEY_RIGHT_BRACKET, prevRB_, [&]{ buildScene(sceneIdx_ + 1); });
    edge(GLFW_KEY_P, prevP_, [&]{
      postFx_ = static_cast<PostFx>((static_cast<int>(postFx_) + 1) % static_cast<int>(PostFx::COUNT));
    });
    edge(GLFW_KEY_N, prevN_, [&]{ showNormals_ = !showNormals_; });
    edge(GLFW_KEY_F, prevF_, [&]{ cullingOn_   = !cullingOn_;   });
    edge(GLFW_KEY_H, prevH_, [&]{ showOverlay_ = !showOverlay_; });

    Material* mats[7] = { matLitWarm_, matLitCool_, matToon_, matRim_, matChecker_, matUnlit_, matWire_ };
    int keys[7] = { GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7 };
    bool* prevs[7] = { &prev1_, &prev2_, &prev3_, &prev4_, &prev5_, &prev6_, &prev7_ };
    for (int i = 0; i < 7; ++i) {
      bool now = window_->keyPressed(keys[i]);
      if (now && !*prevs[i]) scene_.setAllEntitiesMaterial(mats[i]);
      *prevs[i] = now;
    }

    edge(GLFW_KEY_T, prevT_, [&]{ showWireOverlay_ = !showWireOverlay_; });
    edge(GLFW_KEY_B, prevB_, [&]{ showDebugBounds_ = !showDebugBounds_; });
    edge(GLFW_KEY_L, prevL_, [&]{ showDebugLights_ = !showDebugLights_; });
    edge(GLFW_KEY_J, prevJ_, [&]{ shadowsEnabled_  = !shadowsEnabled_;  });
    edge(GLFW_KEY_K, prevK_, [&]{ iblEnabled_      = !iblEnabled_;      });
    edge(GLFW_KEY_V, prevV_, [&]{
      fpsMode_ = static_cast<FpsMode>((static_cast<int>(fpsMode_) + 1)
                                      % static_cast<int>(FpsMode::COUNT));
    });
  }

  // ===== Stats =============================================================
  void updateFrameStats(float dt) {
    frameMs_ = dt * 1000.0f;
    // Exponential moving average over ~half a second.
    const float alpha = 0.1f;
    fpsAvg_ = (1.0f - alpha) * fpsAvg_ + alpha * (1.0f / std::max(dt, 1e-6f));

    overlayTimer_ += dt;
    if (overlayTimer_ >= 0.5f) {
      overlayTimer_ = 0.0f;
      if (gpuMemSupported_) {
        GLint freeKB = 0;
        glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &freeKB);
        if (glGetError() == GL_NO_ERROR) gpuMemFreeKB_ = freeKB;
      }
    }
  }

  // ===== Post-process chain ================================================
  void runFs(FrameBuffer* target, Shader* sh, std::function<void(Shader&)> setU) {
    if (target) target->bind();
    else { FrameBuffer::bindDefault(); glViewport(0, 0, fbWidth_, fbHeight_); }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    sh->bind();
    if (setU) setU(*sh);
    fsTri_.draw();
  }

  void runPostFx() {
    Vec2 texelSize{1.0f / float(fbWidth_), 1.0f / float(fbHeight_)};

    // Each effect writes its (still-linear-HDR) result into pingFbo_ by
    // default; bloom uses pongFbo_ for the final composite and points
    // `result` at it. The final tonemap pass below reads from `result`.
    FrameBuffer* result = &pingFbo_;

    switch (postFx_) {
      case PostFx::None: {
        runFs(&pingFbo_, shPass_, [&](Shader& s){
          glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneFbo_.colorTexture());
          s.setInt("uColor", 0);
          s.setFloat("uVignette", 0.0f);
        });
      } break;

      case PostFx::Sobel: {
        runFs(&pingFbo_, shSobel_, [&](Shader& s){
          glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneFbo_.colorTexture());
          s.setInt("uColor", 0);
          s.setVec2("uTexelSize", texelSize);
        });
      } break;

      case PostFx::Bloom: {
        // 1) Bright extract (HDR threshold > 1.0): scene → ping
        runFs(&pingFbo_, shBright_, [&](Shader& s){
          glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneFbo_.colorTexture());
          s.setInt("uColor", 0);
          s.setFloat("uThreshold", 1.0f);
        });
        // 2) Blur H: ping → pong
        runFs(&pongFbo_, shBlur_, [&](Shader& s){
          glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, pingFbo_.colorTexture());
          s.setInt("uColor", 0);
          s.setVec2("uDirection", Vec2{texelSize.x, 0.0f});
        });
        // 3) Blur V: pong → ping (ping now holds blurred bloom)
        runFs(&pingFbo_, shBlur_, [&](Shader& s){
          glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, pongFbo_.colorTexture());
          s.setInt("uColor", 0);
          s.setVec2("uDirection", Vec2{0.0f, texelSize.y});
        });
        // 4) Composite scene + bloom into pong (so the tonemap below reads pong).
        runFs(&pongFbo_, shComp_, [&](Shader& s){
          glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneFbo_.colorTexture());
          s.setInt("uScene", 0);
          glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, pingFbo_.colorTexture());
          s.setInt("uBloom", 1);
          s.setFloat("uBloomStrength", 0.6f);
          s.setFloat("uVignette", 0.0f);
          glActiveTexture(GL_TEXTURE0);
        });
        result = &pongFbo_;
      } break;

      case PostFx::Fog: {
        runFs(&pingFbo_, shFog_, [&](Shader& s){
          glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneFbo_.colorTexture());
          s.setInt("uColor", 0);
          glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, sceneFbo_.depthTexture());
          s.setInt("uDepth", 1);
          s.setFloat("uNear", scene_.camera.zNear);
          s.setFloat("uFar",  scene_.camera.zFar);
          s.setVec3 ("uFogColor", Vec3{0.5f, 0.55f, 0.65f});
          s.setFloat("uFogStart", 8.0f);
          s.setFloat("uFogEnd",   45.0f);
          glActiveTexture(GL_TEXTURE0);
        });
      } break;

      case PostFx::Ssao: {
        runFs(&pingFbo_, shSsao_, [&](Shader& s){
          glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneFbo_.colorTexture());
          s.setInt("uColor", 0);
          glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, sceneFbo_.depthTexture());
          s.setInt("uDepth", 1);
          s.setVec2 ("uTexelSize", texelSize);
          s.setFloat("uNear", scene_.camera.zNear);
          s.setFloat("uFar",  scene_.camera.zFar);
          s.setFloat("uRadius", 1.5f);
          s.setFloat("uIntensity", 1.6f);
          glActiveTexture(GL_TEXTURE0);
        });
      } break;

      case PostFx::Chromatic: {
        runFs(&pingFbo_, shChrom_, [&](Shader& s){
          glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, sceneFbo_.colorTexture());
          s.setInt("uColor", 0);
          s.setFloat("uAmount", 0.012f);
        });
      } break;

      case PostFx::COUNT: break;
    }

    // Always-on final pass: ACES tonemap + sRGB encode → default framebuffer.
    runFs(nullptr, shTonemap_, [&](Shader& s){
      glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, result->colorTexture());
      s.setInt("uColor", 0);
      s.setFloat("uExposure", exposure_);
      s.setFloat("uVignette", 0.5f);
    });
  }

  // ===== Overlay ===========================================================
  const char* postFxName() const {
    switch (postFx_) {
      case PostFx::None: return "NONE";
      case PostFx::Sobel: return "SOBEL EDGE";
      case PostFx::Bloom: return "BLOOM";
      case PostFx::Fog:   return "DEPTH FOG";
      case PostFx::Ssao:  return "SSAO LITE";
      case PostFx::Chromatic: return "CHROMATIC AB";
      case PostFx::COUNT: break;
    }
    return "?";
  }

  void drawOverlay() {
    const float scale = 2.0f;
    const int   lh    = static_cast<int>((TextRenderer::kCharH + 2) * scale);
    int x = 12;
    int y = 12;
    Vec3 col{0.95f, 1.0f, 0.95f};
    Vec3 dim{0.6f, 0.7f, 0.7f};

    text_.draw("TYRO ENGINE", x, y, scale, Vec3{1.0f, 0.9f, 0.4f}); y += lh;
    const char* lockStr =
        fpsMode_ == FpsMode::Fps120    ? "120"      :
        fpsMode_ == FpsMode::Fps60     ? "60"       :
                                          "UNLOCKED";
    text_.drawf(x, y, scale, col, "FPS %d (%.1f MS)  CAP %s",
                int(fpsAvg_), frameMs_, lockStr); y += lh;
    text_.drawf(x, y, scale, col, "SCENE %d/10 %s", sceneIdx_ + 1, sceneName_); y += lh;
    if (sceneDesc_ && sceneDesc_[0]) {
      text_.drawf(x, y, scale, dim, "  %s", sceneDesc_); y += lh;
    }
    text_.drawf(x, y, scale, col, "POST FX  %s",  postFxName()); y += lh;
    text_.drawf(x, y, scale, col, "ENTITIES %d/%d", visibleCount_, (int)scene_.entities.size()); y += lh;
    text_.drawf(x, y, scale, col, "OCTREE %d NODES", scene_.totalOctreeNodes()); y += lh;
    text_.drawf(x, y, scale, col, "DRAWS %d  TRIS %d", lastDrawCalls_, lastTriangles_); y += lh;
    text_.drawf(x, y, scale, col, "CULL %s  SHADOW %s  IBL %s",
                cullingOn_       ? "ON" : "OFF",
                shadowsEnabled_  ? "ON" : "OFF",
                iblEnabled_      ? "ON" : "OFF"); y += lh;
    text_.drawf(x, y, scale, col, "NORMALS %s  WIRE %s  BOUNDS %s  LIGHTS %s",
                showNormals_     ? "ON" : "OFF",
                showWireOverlay_ ? "ON" : "OFF",
                showDebugBounds_ ? "ON" : "OFF",
                showDebugLights_ ? "ON" : "OFF"); y += lh;

    // GPU timer breakdown.
    Vec3 gpuCol{0.6f, 0.95f, 0.7f};
    double gpuTotal = 0.0;
    for (int i = 0; i < gpuTimer_.sectionCount(); ++i)
      gpuTotal += gpuTimer_.resultMs(i);
    text_.drawf(x, y, scale, gpuCol, "GPU %.2f MS", gpuTotal); y += lh;
    for (int i = 0; i < gpuTimer_.sectionCount(); ++i) {
      text_.drawf(x, y, scale, dim, "  %-7s %.2f", gpuTimer_.name(i).c_str(),
                  gpuTimer_.resultMs(i));
      y += lh;
    }
    if (gpuMemSupported_) {
      int usedMB  = (gpuMemTotalKB_ - gpuMemFreeKB_) / 1024;
      int totalMB = gpuMemTotalKB_ / 1024;
      text_.drawf(x, y, scale, col, "VRAM %d/%d MB", usedMB, totalMB); y += lh;
    }

    // Hint block, bottom-left.
    int hy = fbHeight_ - lh * 14 - 12;
    text_.draw("WASD MOUSE   FLY",         x, hy, scale, dim); hy += lh;
    text_.draw("TAB          CAPTURE",     x, hy, scale, dim); hy += lh;
    text_.draw("[ ]          SCENE",       x, hy, scale, dim); hy += lh;
    text_.draw("P            POSTFX",      x, hy, scale, dim); hy += lh;
    text_.draw("N            NORMALS",     x, hy, scale, dim); hy += lh;
    text_.draw("T            WIREFRAME",   x, hy, scale, dim); hy += lh;
    text_.draw("B            BOUNDS",      x, hy, scale, dim); hy += lh;
    text_.draw("L            LIGHTS",      x, hy, scale, dim); hy += lh;
    text_.draw("J            SHADOWS",     x, hy, scale, dim); hy += lh;
    text_.draw("K            IBL",         x, hy, scale, dim); hy += lh;
    text_.draw("V            FPS CAP",     x, hy, scale, dim); hy += lh;
    text_.draw("F            CULL",        x, hy, scale, dim); hy += lh;
    text_.draw("1-7          MATERIAL",    x, hy, scale, dim); hy += lh;
    text_.draw("H            HIDE OVERLAY",x, hy, scale, dim);

    text_.flush(fbWidth_, fbHeight_);
  }

private:
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
  unsigned int hdrEquirect_  = 0;
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
        * shTonemap_=nullptr;

  Material* matLitWarm_=nullptr, *matLitCool_=nullptr, *matToon_=nullptr,
          * matRim_=nullptr, *matChecker_=nullptr, *matUnlit_=nullptr,
          * matEmissive_=nullptr, *matWire_=nullptr,
          * matWater_=nullptr, *matExplode_=nullptr;
  // Procedural-pattern fragment-shader materials.
  Material* matMarbleProc_=nullptr, *matWoodProc_=nullptr, *matBrickProc_=nullptr,
          * matHexProc_=nullptr,    *matIridescent_=nullptr, *matHologram_=nullptr;
  // Texture-mapped (texture-aware Phong) materials.
  Material* matTexBrick_=nullptr,   *matTexWood_=nullptr,    *matTexMarble_=nullptr,
          * matTexHex_=nullptr,     *matTexChecker_=nullptr;

  // PBR materials.
  Material* matPbrBrick_=nullptr,   *matPbrWood_=nullptr,    *matPbrMarble_=nullptr,
          * matPbrGold_=nullptr,    *matPbrCopper_=nullptr,  *matPbrPlastic_=nullptr;

  // Texture pool (CPU-procedural buffers uploaded to GPU).
  std::vector<std::unique_ptr<Texture>> textures_;
  Texture* texCheckerImg_=nullptr, *texBrickImg_=nullptr, *texWoodImg_=nullptr,
         * texMarbleImg_=nullptr,  *texNoiseImg_=nullptr, *texHexImg_=nullptr,
         * texNormalRough_=nullptr;

  int waterEntityIdx_  = -1;
  int waterFloatStart_ = -1;
  int atriumTorchEntityStart_ = -1;

  enum class PostFx { None, Sobel, Bloom, Fog, Ssao, Chromatic, COUNT };
  PostFx postFx_ = PostFx::None;
  float  exposure_ = 1.0f;
  enum class FpsMode { Fps120, Fps60, Unlocked, COUNT };
  FpsMode fpsMode_ = FpsMode::Fps120;

  std::vector<int> visible_;
  int sceneIdx_ = 0;
  const char* sceneName_ = "?";
  const char* sceneDesc_ = "";
  int lightGardenStart_ = 0;
  bool showNormals_     = false;
  bool showWireOverlay_ = false;
  bool showDebugBounds_ = false;
  bool showDebugLights_ = false;

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
       prevB_=false,   prevL_=false;
  bool prevLB_=false, prevRB_=false, prevH_=false;
};

int main() {
  setvbuf(stderr, nullptr, _IONBF, 0);
  Demo app;
  Engine engine;
  EngineConfig cfg;
  cfg.window.title  = "tyro";
  cfg.window.width  = 1280;
  cfg.window.height = 720;
  return engine.run(app, cfg);
}
