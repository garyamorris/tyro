#pragma once
#include "math/Math.h"

namespace tyro {

enum class LightType : int {
  Directional = 0,
  Point       = 1,
};

struct Light {
  LightType type = LightType::Directional;
  Vec3  position  { 0.0f,  0.0f, 0.0f }; // for point lights
  Vec3  direction { 0.0f, -1.0f, 0.0f }; // for directional (points where the light shines)
  Vec3  color     { 1.0f,  1.0f, 1.0f };
  float intensity = 1.0f;
  float radius    = 10.0f; // attenuation falloff for point lights
};

} // namespace tyro
