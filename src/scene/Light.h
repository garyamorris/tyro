#pragma once
#include "math/Math.h"

namespace tyro {

// Light — POD describing a directional or point source.
//
// Directional: `direction` points where the light *shines* (so shaders use
// `-direction` as the surface-to-light vector). Point: `position` + `radius`;
// shaders apply (1 - d/r)^2 attenuation. The first directional light in
// `Scene::lights` is the one whose view-projection drives the shadow-map pass.

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
