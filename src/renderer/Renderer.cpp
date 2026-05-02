#include "Renderer.h"
#include "gl_loader.h"

namespace tyro {

void Renderer::init() {
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CCW);
}

void Renderer::clear(Vec3 rgb, float /*depth*/) {
  glClearColor(rgb.x, rgb.y, rgb.z, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::setViewport(int x, int y, int w, int h) {
  glViewport(x, y, w, h);
}

void Renderer::enableDepth(bool on)   { on ? glEnable(GL_DEPTH_TEST) : glDisable(GL_DEPTH_TEST); }
void Renderer::enableCulling(bool on) { on ? glEnable(GL_CULL_FACE)  : glDisable(GL_CULL_FACE);  }

} // namespace tyro
