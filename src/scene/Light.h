#pragma once
#include "math/Math.h"

namespace tyro {

// Light — POD describing a directional, point, or spot source.
//
// Directional: `direction` points where the light *shines* (so shaders use
// `-direction` as the surface-to-light vector). Point: `position` + `radius`;
// shaders apply (1 - d/r)^2 attenuation. Spot: `position` + `direction` +
// `radius` + cone angles; shaders combine point-style attenuation with a
// smoothstep cone falloff between innerDeg and outerDeg. The first
// directional light in `Scene::lights` is the one whose view-projection
// drives the shadow-map pass.

enum class LightType : int {
  Directional = 0,
  Point       = 1,
  Spot        = 2,
};

struct Light {
  LightType type = LightType::Directional;
  Vec3  position  { 0.0f,  0.0f, 0.0f }; // point + spot
  Vec3  direction { 0.0f, -1.0f, 0.0f }; // directional + spot (where the light shines)
  Vec3  color     { 1.0f,  1.0f, 1.0f };
  float intensity = 1.0f;
  float radius    = 10.0f; // distance falloff for point + spot

  // Spot-only cone half-angles in degrees. Falloff is smooth (smoothstep)
  // between `innerDeg` (full intensity) and `outerDeg` (zero). Anything
  // outside `outerDeg` is not lit. Ignored for Directional + Point types.
  float innerDeg  = 25.0f;
  float outerDeg  = 35.0f;
};

} // namespace tyro
