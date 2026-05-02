#include "Camera.h"

namespace tyro {

Mat4 Camera::view() const { return lookAt(position, target, up); }
Mat4 Camera::projection() const { return perspective(radians(fovYDeg), aspect, zNear, zFar); }
Mat4 Camera::viewProj() const { return projection() * view(); }
Frustum Camera::frustum() const { return Frustum::fromViewProj(viewProj()); }

} // namespace tyro
