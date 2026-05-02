#pragma once

#include "math/Math.h"

namespace tyro {

class Shader;
class Cubemap;

// Renders a cubemap as the scene background. Intended to be called after the
// main scene pass with depth test = LEQUAL so it fills only un-rasterised
// pixels. The cubemap is sampled with HDR exposure + ACES tonemap to keep the
// look consistent with the main PBR shader.
class Skybox {
public:
  Skybox() = default;
  ~Skybox();
  Skybox(const Skybox&) = delete;
  Skybox& operator=(const Skybox&) = delete;

  bool init();
  void destroy();

  // Reload the skybox shader if its source files changed on disk.
  void reloadShaderIfChanged();

  // Render the cube. Caller is responsible for the FBO, viewport, depth state
  // (recommend GL_LEQUAL + glDepthMask(GL_FALSE)).
  void render(const Cubemap& env, const Mat4& view, const Mat4& proj,
              float exposure = 1.0f);

private:
  unsigned int vao_ = 0, vbo_ = 0;
  Shader* shader_ = nullptr; // owned
};

} // namespace tyro
