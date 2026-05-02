#include "FlyCamera.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <cmath>

namespace tyro {

void FlyCamera::setLook(Vec3 from, Vec3 to) {
  position = from;
  Vec3 d = normalize(to - from);
  pitch = std::asin(d.y);
  yaw   = std::atan2(d.x, -d.z);
}

Vec3 FlyCamera::forward() const {
  float cp = std::cos(pitch), sp = std::sin(pitch);
  float cy = std::cos(yaw),   sy = std::sin(yaw);
  return Vec3{ sy * cp, sp, -cy * cp };
}
Vec3 FlyCamera::right() const {
  float cy = std::cos(yaw), sy = std::sin(yaw);
  return Vec3{ cy, 0.0f, sy };
}

void FlyCamera::update(Window& window, float dt) {
  // Mouse delta — only consume when cursor is captured (mouse-look on).
  double mx = 0, my = 0;
  window.cursorPos(mx, my);
  if (window.cursorCaptured()) {
    if (firstMouse_) { prevMouseX_ = mx; prevMouseY_ = my; firstMouse_ = false; }
    double dx = mx - prevMouseX_;
    double dy = my - prevMouseY_;
    prevMouseX_ = mx; prevMouseY_ = my;
    yaw   += static_cast<float>(dx) * mouseSensitivity;
    pitch -= static_cast<float>(dy) * mouseSensitivity;
    const float kClamp = radians(89.0f);
    if (pitch >  kClamp) pitch =  kClamp;
    if (pitch < -kClamp) pitch = -kClamp;
  } else {
    firstMouse_ = true;
  }

  float moveSpeed = speed;
  if (window.keyPressed(GLFW_KEY_LEFT_SHIFT)) moveSpeed *= speedMultiplier;
  Vec3 fwd = forward();
  Vec3 rgt = right();
  Vec3 worldUp{0, 1, 0};

  Vec3 delta{0,0,0};
  if (window.keyPressed(GLFW_KEY_W)) delta = delta + fwd;
  if (window.keyPressed(GLFW_KEY_S)) delta = delta - fwd;
  if (window.keyPressed(GLFW_KEY_D)) delta = delta + rgt;
  if (window.keyPressed(GLFW_KEY_A)) delta = delta - rgt;
  if (window.keyPressed(GLFW_KEY_E) || window.keyPressed(GLFW_KEY_SPACE))
    delta = delta + worldUp;
  if (window.keyPressed(GLFW_KEY_Q) || window.keyPressed(GLFW_KEY_LEFT_CONTROL))
    delta = delta - worldUp;

  if (length(delta) > 0.0f) delta = normalize(delta);
  position = position + delta * (moveSpeed * dt);
}

void FlyCamera::apply(Camera& cam) const {
  cam.position = position;
  cam.target   = position + forward();
  cam.up       = Vec3{0, 1, 0};
}

} // namespace tyro
