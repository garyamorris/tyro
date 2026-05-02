#include "Engine.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <chrono>
#include <thread>

#ifdef _WIN32
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  #include <timeapi.h>
  #pragma comment(lib, "winmm")
#endif

namespace tyro {

namespace {

// Sleep + spin until glfwGetTime() reaches `targetTime`. Bulk-sleeps with
// std::this_thread::sleep_for and busy-waits the last fraction of a millisecond
// for sub-millisecond accuracy. Requires Windows timer resolution to be raised
// (timeBeginPeriod(1) below) for sleep_for to wake on time.
void waitUntil(double targetTime) {
  double now = glfwGetTime();
  double remaining = targetTime - now;
  if (remaining > 0.002) {
    auto us = static_cast<long long>((remaining - 0.001) * 1.0e6);
    std::this_thread::sleep_for(std::chrono::microseconds(us));
  }
  while (glfwGetTime() < targetTime) { /* spin */ }
}

} // namespace

int Engine::run(Application& app, const EngineConfig& cfg) {
#ifdef _WIN32
  timeBeginPeriod(1);
#endif

  Window window;
  if (!window.create(cfg.window)) {
#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 1;
  }
  if (!app.onInit(window)) {
#ifdef _WIN32
    timeEndPeriod(1);
#endif
    return 2;
  }

  int prevW = 0, prevH = 0;
  window.framebufferSize(prevW, prevH);
  app.onResize(prevW, prevH);

  double prevTime = glfwGetTime();
  double accumulator = 0.0;

  while (!window.shouldClose()) {
    double frameStart = glfwGetTime();
    window.pollEvents();

    int w = 0, h = 0;
    window.framebufferSize(w, h);
    if (w != prevW || h != prevH) {
      prevW = w; prevH = h;
      app.onResize(w, h);
    }

    double frameTime = frameStart - prevTime;
    if (frameTime > 0.25) frameTime = 0.25; // clamp big stalls (debugger pauses)
    prevTime = frameStart;
    accumulator += frameTime;

    int steps = 0;
    while (accumulator >= cfg.fixedDt && steps < cfg.maxStepsPerFrame) {
      app.onUpdate(cfg.fixedDt);
      accumulator -= cfg.fixedDt;
      ++steps;
    }

    float alpha = static_cast<float>(accumulator / cfg.fixedDt);
    app.onRender(alpha);
    window.swapBuffers();

    // Frame-rate cap. The deadline is computed from the start of THIS frame so
    // we don't accumulate debt — a single slow frame doesn't trigger a
    // catch-up sprint.
    double targetFps = app.frameRateTarget();
    if (targetFps > 0.0) {
      waitUntil(frameStart + 1.0 / targetFps);
    }
  }

  app.onShutdown();
#ifdef _WIN32
  timeEndPeriod(1);
#endif
  return 0;
}

} // namespace tyro
