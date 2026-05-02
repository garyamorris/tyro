#pragma once
#include "window/Window.h"

namespace tyro {

// Game application interface — subclass and override the hooks.
class Application {
public:
  virtual ~Application() = default;
  virtual bool onInit(Window& w) { (void)w; return true; }
  virtual void onUpdate(float dt) { (void)dt; }     // fixed timestep
  virtual void onRender(float alpha) { (void)alpha; } // alpha = interpolation factor
  virtual void onResize(int w, int h) { (void)w; (void)h; }
  virtual void onShutdown() {}

  // Cap on render rate, in frames per second. 0 means unlimited (the engine
  // won't sleep between frames). The Engine queries this every frame so the
  // app can flip it at runtime.
  virtual double frameRateTarget() const { return 0.0; }
};

struct EngineConfig {
  WindowConfig window{};
  float fixedDt = 1.0f / 120.0f; // seconds per fixed-step update
  int   maxStepsPerFrame = 5;
};

class Engine {
public:
  int run(Application& app, const EngineConfig& cfg = {});
};

} // namespace tyro
