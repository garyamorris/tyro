#pragma once
#include "math/Math.h"

namespace tyro {

// Light-weight wrapper for state we set per-frame.
class Renderer {
public:
  void init();
  void clear(Vec3 rgb, float depth = 1.0f);
  void setViewport(int x, int y, int w, int h);
  void enableDepth(bool on);
  void enableCulling(bool on);
};

} // namespace tyro
