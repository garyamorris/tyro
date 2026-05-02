#include "Skybox.h"

#include "Cubemap.h"
#include "Shader.h"
#include "gl_loader.h"

namespace tyro {

namespace {
const float kCubeVerts[] = {
  -1,-1,-1,  1, 1,-1,  1,-1,-1,
   1, 1,-1, -1,-1,-1, -1, 1,-1,
  -1,-1, 1,  1,-1, 1,  1, 1, 1,
   1, 1, 1, -1, 1, 1, -1,-1, 1,
  -1, 1, 1, -1, 1,-1, -1,-1,-1,
  -1,-1,-1, -1,-1, 1, -1, 1, 1,
   1, 1, 1,  1,-1,-1,  1, 1,-1,
   1,-1,-1,  1, 1, 1,  1,-1, 1,
  -1,-1,-1,  1,-1,-1,  1,-1, 1,
   1,-1, 1, -1,-1, 1, -1,-1,-1,
  -1, 1,-1,  1, 1, 1,  1, 1,-1,
   1, 1, 1, -1, 1,-1, -1, 1, 1,
};
} // namespace

Skybox::~Skybox() { destroy(); }

bool Skybox::init() {
  shader_ = new Shader();
  if (!shader_->loadFromFiles("shaders/skybox.vert", "shaders/skybox.frag")) {
    delete shader_; shader_ = nullptr;
    return false;
  }
  glGenVertexArrays(1, &vao_);
  glBindVertexArray(vao_);
  glGenBuffers(1, &vbo_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glBindVertexArray(0);
  return true;
}

void Skybox::destroy() {
  if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
  if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
  if (shader_) { delete shader_; shader_ = nullptr; }
}

void Skybox::reloadShaderIfChanged() {
  if (shader_) shader_->reloadIfChanged();
}

void Skybox::render(const Cubemap& env, const Mat4& view, const Mat4& proj,
                    float exposure) {
  if (!shader_ || !env.valid()) return;
  shader_->bind();
  shader_->setMat4 ("uView", view);
  shader_->setMat4 ("uProj", proj);
  shader_->setFloat("uExposure", exposure);

  glActiveTexture(GL_TEXTURE0);
  env.bind();
  shader_->setInt("uEnv", 0);

  glBindVertexArray(vao_);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glBindVertexArray(0);
}

} // namespace tyro
