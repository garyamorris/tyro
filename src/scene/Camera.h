#pragma once
#include "math/Math.h"
#include "math/Frustum.h"

namespace tyro {

class Camera {
public:
  Vec3  position { 0.0f, 0.0f, 5.0f };
  Vec3  target   { 0.0f, 0.0f, 0.0f };
  Vec3  up       { 0.0f, 1.0f, 0.0f };

  // Frustum settings.
  float fovYDeg = 60.0f;
  float aspect  = 16.0f / 9.0f;
  float zNear   = 0.1f;
  float zFar    = 100.0f;

  Mat4    view()      const;
  Mat4    projection() const;
  Mat4    viewProj()  const;
  Frustum frustum()   const;
};

} // namespace tyro
