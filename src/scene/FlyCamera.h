#pragma once
#include "math/Math.h"
#include "scene/Camera.h"
#include "window/Window.h"

namespace tyro {

// FlyCamera — first-person free-look controller.
//
// update() reads mouse delta into yaw/pitch and WASD/Q-E into position
// (Shift sprints). The cursor must be captured for mouse-look to work — see
// Window::setCursorCaptured. Call apply() each frame to push the resulting
// eye/target/up into a Camera that the renderer will then read.

class FlyCamera {
public:
  Vec3  position { 0.0f, 1.5f, 6.0f };
  float yaw   = 0.0f;   // rotation about Y (radians, 0 looks down -Z)
  float pitch = 0.0f;   // rotation about X (radians, clamped)

  float speed             = 6.0f;
  float speedMultiplier   = 5.0f;  // when Shift held
  float mouseSensitivity  = 0.0025f;

  void setLook(Vec3 from, Vec3 to);
  void update(Window& window, float dt);
  void apply(Camera& cam) const;

  Vec3 forward() const;
  Vec3 right()   const;

private:
  bool   firstMouse_ = true;
  double prevMouseX_ = 0.0;
  double prevMouseY_ = 0.0;
};

} // namespace tyro
