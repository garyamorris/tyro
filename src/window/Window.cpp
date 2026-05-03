#include "Window.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cstdio>

#include "gl_loader.h"

namespace tyro {

int Window::refCount_ = 0;

static void glfwErrorCb(int code, const char* msg) {
  std::fprintf(stderr, "[glfw] error %d: %s\n", code, msg);
}

bool Window::create(const WindowConfig& cfg) {
  if (refCount_ == 0) {
    glfwSetErrorCallback(glfwErrorCb);
    if (!glfwInit()) return false;
  }
  ++refCount_;

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window_ = glfwCreateWindow(cfg.width, cfg.height, cfg.title.c_str(), nullptr, nullptr);
  if (!window_) return false;

  glfwMakeContextCurrent(window_);
  // Vsync disabled — frame pacing is handled by the Engine's frame limiter so
  // we can hit arbitrary targets (e.g. 120 FPS) on any monitor refresh rate.
  glfwSwapInterval(0);

  if (!tyro_gl_load(reinterpret_cast<tyro_gl_proc_loader>(glfwGetProcAddress))) {
    std::fprintf(stderr, "[window] failed to load required GL functions\n");
    return false;
  }

  std::fprintf(stderr, "[gl] %s | %s | %s\n",
               glGetString(GL_VENDOR), glGetString(GL_RENDERER), glGetString(GL_VERSION));
  return true;
}

Window::~Window() { destroy(); }

void Window::destroy() {
  if (window_) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
    if (--refCount_ == 0) glfwTerminate();
  }
}

bool Window::shouldClose() const { return glfwWindowShouldClose(window_); }
void Window::pollEvents() const  { glfwPollEvents(); }
void Window::swapBuffers() const { glfwSwapBuffers(window_); }

void Window::framebufferSize(int& w, int& h) const {
  glfwGetFramebufferSize(window_, &w, &h);
}

bool Window::keyPressed(int key) const {
  return glfwGetKey(window_, key) == GLFW_PRESS;
}

void Window::cursorPos(double& x, double& y) const {
  glfwGetCursorPos(window_, &x, &y);
}

bool Window::mouseButtonPressed(int button) const {
  return glfwGetMouseButton(window_, button) == GLFW_PRESS;
}

void Window::setCursorCaptured(bool captured) {
  if (!window_) return;
  glfwSetInputMode(window_, GLFW_CURSOR,
                   captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
  cursorCaptured_ = captured;
}

} // namespace tyro
