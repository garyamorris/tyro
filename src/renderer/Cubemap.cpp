#include "Cubemap.h"
#include "gl_loader.h"

namespace tyro {

Cubemap::~Cubemap() { destroy(); }

void Cubemap::destroy() {
  if (tex_) { glDeleteTextures(1, &tex_); tex_ = 0; }
  faceSize_ = 0; mipLevels_ = 1;
}

bool Cubemap::create(int faceSize, unsigned int internalFormat,
                     unsigned int format, unsigned int type,
                     int mipLevels, bool linear) {
  destroy();
  faceSize_  = faceSize;
  mipLevels_ = mipLevels < 1 ? 1 : mipLevels;

  glGenTextures(1, &tex_);
  glBindTexture(GL_TEXTURE_CUBE_MAP, tex_);

  // Allocate every face / every mip with TexImage2D (no immutable storage on
  // GL 3.3 core; this is the standard portable path).
  for (int mip = 0; mip < mipLevels_; ++mip) {
    int s = faceSize_ >> mip;
    if (s < 1) s = 1;
    for (int f = 0; f < 6; ++f) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + f, mip,
                   static_cast<GLint>(internalFormat),
                   s, s, 0, format, type, nullptr);
    }
  }

  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  GLenum minF = (mipLevels_ > 1)
              ? (linear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST)
              : (linear ? GL_LINEAR              : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, minF);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER,
                  linear ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_BASE_LEVEL, 0);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAX_LEVEL,  mipLevels_ - 1);
  return true;
}

void Cubemap::generateMips() {
  if (!tex_) return;
  glBindTexture(GL_TEXTURE_CUBE_MAP, tex_);
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
}

void Cubemap::bind() const {
  glBindTexture(GL_TEXTURE_CUBE_MAP, tex_);
}

void Cubemap::attachFace(int face, int mip) const {
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
                         tex_, mip);
}

} // namespace tyro
