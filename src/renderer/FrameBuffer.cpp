#include "FrameBuffer.h"
#include "gl_loader.h"
#include <cstdio>

namespace tyro {

FrameBuffer::~FrameBuffer() { destroy(); }

void FrameBuffer::destroy() {
  if (depthRbo_) { glDeleteRenderbuffers(1, &depthRbo_); depthRbo_ = 0; }
  if (depthTex_) { glDeleteTextures(1, &depthTex_); depthTex_ = 0; }
  if (colorTex_) { glDeleteTextures(1, &colorTex_); colorTex_ = 0; }
  if (fbo_)      { glDeleteFramebuffers(1, &fbo_); fbo_ = 0; }
  width_ = height_ = 0;
}

bool FrameBuffer::create(int width, int height, DepthMode depth, bool hdr) {
  destroy();
  width_ = width; height_ = height;
  depthMode_ = depth;
  hdr_       = hdr;

  glGenFramebuffers(1, &fbo_);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

  glGenTextures(1, &colorTex_);
  glBindTexture(GL_TEXTURE_2D, colorTex_);
  GLenum internal = hdr ? GL_RGBA16F : GL_RGBA8;
  GLenum dataType = hdr ? GL_FLOAT   : GL_UNSIGNED_BYTE;
  glTexImage2D(GL_TEXTURE_2D, 0, internal, width, height, 0,
               GL_RGBA, dataType, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, colorTex_, 0);

  if (depth == DepthMode::Renderbuffer) {
    glGenRenderbuffers(1, &depthRbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, depthRbo_);
  } else if (depth == DepthMode::Texture) {
    glGenTextures(1, &depthTex_);
    glBindTexture(GL_TEXTURE_2D, depthTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                           GL_TEXTURE_2D, depthTex_, 0);
  }

  GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  if (status != GL_FRAMEBUFFER_COMPLETE) {
    std::fprintf(stderr, "[fbo] incomplete (0x%x)\n", status);
    return false;
  }
  return true;
}

void FrameBuffer::resize(int width, int height) {
  if (width == width_ && height == height_) return;
  create(width, height, depthMode_, hdr_);
}

void FrameBuffer::bind() const {
  glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
  glViewport(0, 0, width_, height_);
}

void FrameBuffer::bindDefault() {
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

} // namespace tyro
