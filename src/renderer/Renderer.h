#pragma once
#include "math/Math.h"

namespace tyro {

// Renderer — thin wrapper for per-frame GL state we toggle from C++.
//
// Just enough to keep raw glEnable / glClear / glViewport calls out of
// application code. The "real" rendering is split across Scene::render
// (the main pass), Skybox, the post-FX shaders, and TextRenderer.
class Renderer {
public:
  void init();
  void clear(Vec3 rgb, float depth = 1.0f);
  void setViewport(int x, int y, int w, int h);
  void enableDepth(bool on);
  void enableCulling(bool on);
};

} // namespace tyro
