#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tyro {

// 2D texture wrapper. Supports loading from file (stb_image) or raw memory
// (procedural). Default sampling: trilinear with mipmaps, GL_REPEAT wrap.
class Texture {
public:
  struct Options {
    bool generateMipmaps = true;
    bool srgb            = false; // future hook
    int  wrapS           = 0;     // 0 = REPEAT (default), 1 = CLAMP_TO_EDGE
    int  wrapT           = 0;
    bool filterLinear    = true;
  };

  Texture() = default;
  ~Texture();
  Texture(const Texture&) = delete;
  Texture& operator=(const Texture&) = delete;
  Texture(Texture&& o) noexcept { *this = static_cast<Texture&&>(o); }
  Texture& operator=(Texture&& o) noexcept;

  bool loadFromFile  (const char* path);
  bool loadFromFile  (const char* path, const Options& opts);
  bool loadFromMemory(int width, int height, int channels,
                      const std::uint8_t* pixels);
  bool loadFromMemory(int width, int height, int channels,
                      const std::uint8_t* pixels, const Options& opts);

  void destroy();
  unsigned int handle() const { return tex_; }
  int  width()  const { return width_; }
  int  height() const { return height_; }
  bool valid()  const { return tex_ != 0; }

private:
  unsigned int tex_  = 0;
  int          width_ = 0, height_ = 0;
  void apply(const Options& opts);
};

} // namespace tyro
