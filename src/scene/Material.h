#pragma once
#include "math/Math.h"

namespace tyro {

class Shader;
class Texture;

// A small, data-driven material. The Renderer picks up whatever uniforms the
// material's shader actually declares — unused values are silently ignored.
//
// Texture bindings established by apply():
//   TEX0 = albedoTex
//   TEX1 = normalTex   (PBR)
//   TEX2 = mrTex       (PBR — R: metallic, G: roughness — glTF-ish)
//   TEX3 = shadow map  (set by Scene::uploadSceneUniforms, not here)
//   TEX5 = irradiance cubemap (Scene-bound, IBL)
//   TEX6 = prefiltered radiance cubemap (Scene-bound, IBL)
//   TEX7 = BRDF integration LUT (Scene-bound, IBL)
struct Material {
  Shader*  shader      = nullptr;
  Vec3     albedo      { 0.8f, 0.8f, 0.8f };
  Vec3     emissive    { 0.0f, 0.0f, 0.0f };
  float    shininess   = 32.0f;     // legacy Phong term
  bool     doubleSided = false;

  // PBR scalars (uniform fallback when no texture is bound).
  float    metallic    = 0.0f;
  float    roughness   = 0.5f;

  // Texture maps — any may be null.
  Texture* albedoTex   = nullptr;
  Texture* normalTex   = nullptr;
  Texture* mrTex       = nullptr;
  Vec2     uvScale     { 1.0f, 1.0f };
  Vec2     uvOffset    { 0.0f, 0.0f };

  void apply() const;
};

} // namespace tyro
