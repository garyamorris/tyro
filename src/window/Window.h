#pragma once
#include <string>

struct GLFWwindow;

namespace tyro {

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
  void setCursorCaptured(bool captured);
  bool cursorCaptured() const { return cursorCaptured_; }

  GLFWwindow* handle() const { return window_; }

private:
  GLFWwindow* window_ = nullptr;
  bool        cursorCaptured_ = false;
  static int  refCount_;
};

} // namespace tyro
