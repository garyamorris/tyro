#pragma once
#include <cstdint>

namespace tyro {

// A GL_TEXTURE_CUBE_MAP wrapper used as a render target during the IBL bake
// (env capture, irradiance convolution, prefiltered radiance) and as a
// sampleable input later.
//
// The bake passes attach a single face/mip via attachFace() to a caller-owned
// FBO and render a unit-cube; this avoids needing layered framebuffer support.
class Cubemap {
public:
  Cubemap() = default;
  ~Cubemap();
  Cubemap(const Cubemap&) = delete;
  Cubemap& operator=(const Cubemap&) = delete;

  // Allocate an empty cubemap. `internalFormat` is e.g. GL_RGB16F. Pass
  // mipLevels > 1 to allocate a fixed mip chain (used for the prefilter map).
  // `linear` chooses LINEAR (true) or NEAREST sampling. With mipLevels > 1 the
  // min filter is set to LINEAR_MIPMAP_LINEAR.
  bool create(int faceSize, unsigned int internalFormat,
              unsigned int format, unsigned int type,
              int mipLevels = 1, bool linear = true);

  // Generate the mip chain from the level-0 contents (used after equirect→cube
  // capture so the prefilter pass can sample lower mips for low roughness).
  void generateMips();

  void destroy();

  // Bind the entire cube to GL_TEXTURE_CUBE_MAP on the currently active unit.
  void bind() const;

  unsigned int handle() const { return tex_; }
  int faceSize() const { return faceSize_; }
  int mipLevels() const { return mipLevels_; }
  bool valid() const { return tex_ != 0; }

  // For the bake: attach face `face` (0..5) at mip `mip` to GL_FRAMEBUFFER's
  // GL_COLOR_ATTACHMENT0. Caller must have an FBO bound.
  void attachFace(int face, int mip = 0) const;

private:
  unsigned int tex_ = 0;
  int faceSize_ = 0;
  int mipLevels_ = 1;
};

} // namespace tyro
