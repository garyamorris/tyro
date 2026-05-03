#pragma once
#include <string>

struct GLFWwindow;

namespace tyro {

// Window — thin GLFW wrapper.
//
// Owns one GLFWwindow with an OpenGL 3.3 core context. Vsync is left off so
// the Engine can do its own frame-rate cap. Tracks mouse-capture state for
// FlyCamera and reference-counts GLFW's global init/terminate so a future
// multi-window setup wouldn't double-init.

struct WindowConfig {
  int width  = 1280;
  int height = 720;
  std::string title = "tyro";
};

class Window {
public:
  Window() = default;
  ~Window();
  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  bool create(const WindowConfig& cfg);
  void destroy();

  bool shouldClose() const;
  void pollEvents() const;
  void swapBuffers() const;

  void framebufferSize(int& w, int& h) const;
  bool keyPressed(int glfwKey) const;

  // Mouse / cursor.
  void cursorPos(double& x, double& y) const;
  bool mouseButtonPressed(int glfwButton) const;
  void setCursorCaptured(bool captured);
  bool cursorCaptured() const { return cursorCaptured_; }

  GLFWwindow* handle() const { return window_; }

private:
  GLFWwindow* window_ = nullptr;
  bool        cursorCaptured_ = false;
  static int  refCount_;
};

} // namespace tyro
