#pragma once
#include "window/Window.h"

namespace tyro {

// Engine — the central frame loop.
//
// `Engine::run(app, cfg)` owns a Window, then drives the loop:
//   poll input  →  run zero-or-more fixed-timestep onUpdate(dt) calls  →
//   call onRender(alpha) once  →  swapBuffers  →  frame-rate cap.
//
// The fixed-timestep accumulator is a Glenn-Fiedler integrator: simulation
// runs at a stable rate (cfg.fixedDt) regardless of render rate, so physics
// and animation stay deterministic at any FPS. `alpha` is the interpolation
// factor in [0, 1) between the last and next simulation step — render code
// can use it to smooth visual updates.
//
// To use: subclass Application, override the hooks you need, hand it to
// Engine::run(). Everything else is the engine's job.

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
