#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "math/Math.h"

namespace tyro {

class Shader;

// Cheap immediate-mode text renderer driven by a 5x7 embedded bitmap font.
// All draw() calls within a frame are batched and flushed in flush(),
// producing a single draw call.
class TextRenderer {
public:
  TextRenderer() = default;
  ~TextRenderer();
  TextRenderer(const TextRenderer&) = delete;
  TextRenderer& operator=(const TextRenderer&) = delete;

  bool init();

  // x, y in pixels, top-left origin; scale = pixels per font-pixel.
  void draw(const char* text, int x, int y, float scale,
            Vec3 color = Vec3{1,1,1});
  void drawf(int x, int y, float scale, Vec3 color, const char* fmt, ...);

  // Submit batched quads to the GPU. Caller must have set viewport.
  void flush(int screenW, int screenH);

  void reloadShaderIfChanged();

  // Cell metrics in source pixels.
  static constexpr int kCharW = 5;
  static constexpr int kCharH = 7;
  static constexpr int kCellW = 6;
  static constexpr int kCellH = 8;

private:
  unsigned int atlasTex_ = 0;
  unsigned int vao_ = 0;
  unsigned int vbo_ = 0;
  Shader*      shader_ = nullptr;
  std::vector<float> verts_;        // pos.xy, uv.xy, col.rgb -> 7 floats per vertex
};

} // namespace tyro
