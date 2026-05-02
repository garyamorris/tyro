#pragma once
#include "math/Math.h"

namespace tyro {

class Shader;
class Texture;

// A small, data-driven material. The Renderer picks up whatever uniforms the
// material's shader actually declares — unused values are silently ignored.
struct Material {
  Shader*  shader      = nullptr;
  Vec3     albedo      { 0.8f, 0.8f, 0.8f };
  Vec3     emissive    { 0.0f, 0.0f, 0.0f };
  float    shininess   = 32.0f;
  bool     doubleSided = false;

  // Optional albedo texture. When non-null, the material's shader is expected
  // to multiply uAlbedo by texture(uAlbedoTex, vUV * uUvScale + uUvOffset).
  // Bound to texture unit 0 by apply().
  Texture* albedoTex   = nullptr;
  Vec2     uvScale     { 1.0f, 1.0f };
  Vec2     uvOffset    { 0.0f, 0.0f };

  void apply() const;
};

} // namespace tyro
