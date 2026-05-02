#pragma once
#include "math/Math.h"
#include "math/Frustum.h"

namespace tyro {

// Camera — view + projection state for the scene pass.
//
// Position / target / up generate the view matrix; fovYDeg / aspect / zNear /
// zFar generate a right-handed perspective projection (depth range [-1, 1],
// matching GL convention). frustum() extracts the six culling planes from
// proj * view for octree culling. FlyCamera writes back into one of these
// each frame.

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
