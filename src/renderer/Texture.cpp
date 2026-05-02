#include "Texture.h"
#include "gl_loader.h"

#include <cstdio>

#include "stb_image.h"

namespace tyro {

Texture::~Texture() { destroy(); }

Texture& Texture::operator=(Texture&& o) noexcept {
  if (this != &o) {
    destroy();
    tex_ = o.tex_; width_ = o.width_; height_ = o.height_;
    o.tex_ = 0; o.width_ = 0; o.height_ = 0;
  }
  return *this;
}

void Texture::destroy() {
  if (tex_) { glDeleteTextures(1, &tex_); tex_ = 0; }
  width_ = 0; height_ = 0;
}

void Texture::apply(const Options& opts) {
  glBindTexture(GL_TEXTURE_2D, tex_);
  GLenum wrap = (opts.wrapS == 0) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
  wrap = (opts.wrapT == 0) ? GL_REPEAT : GL_CLAMP_TO_EDGE;
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

  GLenum minF = opts.generateMipmaps
              ? (opts.filterLinear ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST)
              : (opts.filterLinear ? GL_LINEAR              : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minF);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                  opts.filterLinear ? GL_LINEAR : GL_NEAREST);
}

bool Texture::loadFromFile(const char* path, const Options& opts) {
  int w = 0, h = 0, c = 0;
  stbi_set_flip_vertically_on_load(1);
  unsigned char* px = stbi_load(path, &w, &h, &c, 4);
  if (!px) {
    std::fprintf(stderr, "[texture] cannot load %s (%s)\n", path, stbi_failure_reason());
    return false;
  }
  bool ok = loadFromMemory(w, h, 4, px, opts);
  stbi_image_free(px);
  return ok;
}

bool Texture::loadFromMemory(int w, int h, int channels,
                             const std::uint8_t* pixels, const Options& opts) {
  destroy();
  width_ = w; height_ = h;

  glGenTextures(1, &tex_);
  glBindTexture(GL_TEXTURE_2D, tex_);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
  GLenum internal = (channels == 4) ? GL_RGBA8 : GL_RGB8;
  glTexImage2D(GL_TEXTURE_2D, 0, internal, w, h, 0,
               format, GL_UNSIGNED_BYTE, pixels);

  apply(opts);
  if (opts.generateMipmaps) glGenerateMipmap(GL_TEXTURE_2D);
  return true;
}

} // namespace tyro
