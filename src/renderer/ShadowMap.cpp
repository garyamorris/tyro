#include "ShadowMap.h"
#include "gl_loader.h"

#include <cstdio>

namespace tyro {

ShadowMap::~ShadowMap() { destroy(); }

void ShadowMap::destroy() {
  if (depthTex_) { glDeleteTextures(1, &depthTex_); depthTex_ = 0; }
  if (fbo_)      { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
  size_ = 0;
}

bool ShadowMap::create(int size) {
  destroy();
  size_ = size;

  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

  glGenTextures(1, &depthTex_);
  glBindTexture(GL_TEXTURE_2D, depthTex_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, size, size, 0,
               GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                         GL_TEXTURE_2D, depthTex_, 0);

  // Depth-only — explicitly disable color reads/writes for this FBO.
  glDrawBuffer(GL_NONE);
  glReadBuffer(GL_NONE);

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::fprintf(stderr, "[shadowmap] incomplete (0x%x)\n", status);
    return false;
  }
  return true;
}

void ShadowMap::bind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, size_, size_);
}

} // namespace tyro
