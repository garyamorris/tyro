#include "TextRenderer.h"

#include "renderer/Shader.h"
#include "gl_loader.h"

#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstring>

namespace tyro {

// ---------------------------------------------------------------------------
// 5x7 bitmap font. Each glyph is 7 rows of 5 chars; '#' = pixel on, anything
// else (typically '.') = off. Characters not listed render as blanks.
// ---------------------------------------------------------------------------
namespace {

struct GlyphInit {
  char c;
  const char* rows[7];
};

constexpr GlyphInit kGlyphs[] = {
  // --- digits ----------------------------------------------------------
  { '0', { ".###.", "#...#", "#..##", "#.#.#", "##..#", "#...#", ".###." } },
  { '1', { "..#..", ".##..", "..#..", "..#..", "..#..", "..#..", ".###." } },
  { '2', { ".###.", "#...#", "....#", "...#.", "..#..", ".#...", "#####" } },
  { '3', { "#####", "....#", "...#.", "..##.", "....#", "#...#", ".###." } },
  { '4', { "...#.", "..##.", ".#.#.", "#..#.", "#####", "...#.", "...#." } },
  { '5', { "#####", "#....", "####.", "....#", "....#", "#...#", ".###." } },
  { '6', { ".###.", "#...#", "#....", "####.", "#...#", "#...#", ".###." } },
  { '7', { "#####", "....#", "...#.", "..#..", ".#...", ".#...", ".#..." } },
  { '8', { ".###.", "#...#", "#...#", ".###.", "#...#", "#...#", ".###." } },
  { '9', { ".###.", "#...#", "#...#", ".####", "....#", "#...#", ".###." } },
  // --- uppercase letters ----------------------------------------------
  { 'A', { ".###.", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
  { 'B', { "####.", "#...#", "#...#", "####.", "#...#", "#...#", "####." } },
  { 'C', { ".###.", "#...#", "#....", "#....", "#....", "#...#", ".###." } },
  { 'D', { "####.", "#...#", "#...#", "#...#", "#...#", "#...#", "####." } },
  { 'E', { "#####", "#....", "#....", "####.", "#....", "#....", "#####" } },
  { 'F', { "#####", "#....", "#....", "####.", "#....", "#....", "#...." } },
  { 'G', { ".###.", "#...#", "#....", "#.###", "#...#", "#...#", ".###." } },
  { 'H', { "#...#", "#...#", "#...#", "#####", "#...#", "#...#", "#...#" } },
  { 'I', { ".###.", "..#..", "..#..", "..#..", "..#..", "..#..", ".###." } },
  { 'J', { "..###", "...#.", "...#.", "...#.", "...#.", "#..#.", ".##.." } },
  { 'K', { "#...#", "#..#.", "#.#..", "##...", "#.#..", "#..#.", "#...#" } },
  { 'L', { "#....", "#....", "#....", "#....", "#....", "#....", "#####" } },
  { 'M', { "#...#", "##.##", "#.#.#", "#.#.#", "#...#", "#...#", "#...#" } },
  { 'N', { "#...#", "##..#", "#.#.#", "#.#.#", "#..##", "#...#", "#...#" } },
  { 'O', { ".###.", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
  { 'P', { "####.", "#...#", "#...#", "####.", "#....", "#....", "#...." } },
  { 'Q', { ".###.", "#...#", "#...#", "#...#", "#.#.#", "#..#.", ".##.#" } },
  { 'R', { "####.", "#...#", "#...#", "####.", "#.#..", "#..#.", "#...#" } },
  { 'S', { ".###.", "#...#", "#....", ".###.", "....#", "#...#", ".###." } },
  { 'T', { "#####", "..#..", "..#..", "..#..", "..#..", "..#..", "..#.." } },
  { 'U', { "#...#", "#...#", "#...#", "#...#", "#...#", "#...#", ".###." } },
  { 'V', { "#...#", "#...#", "#...#", "#...#", "#...#", ".#.#.", "..#.." } },
  { 'W', { "#...#", "#...#", "#...#", "#.#.#", "#.#.#", "##.##", "#...#" } },
  { 'X', { "#...#", "#...#", ".#.#.", "..#..", ".#.#.", "#...#", "#...#" } },
  { 'Y', { "#...#", "#...#", ".#.#.", "..#..", "..#..", "..#..", "..#.." } },
  { 'Z', { "#####", "....#", "...#.", "..#..", ".#...", "#....", "#####" } },
  // --- punctuation -----------------------------------------------------
  { '.', { ".....", ".....", ".....", ".....", ".....", "..#..", "..#.." } },
  { ',', { ".....", ".....", ".....", ".....", "..#..", "..#..", ".#..." } },
  { ':', { ".....", ".....", "..#..", ".....", ".....", "..#..", "....." } },
  { ';', { ".....", ".....", "..#..", ".....", "..#..", "..#..", ".#..." } },
  { '/', { "....#", "....#", "...#.", "..#..", ".#...", "#....", "#...." } },
  { '-', { ".....", ".....", ".....", "#####", ".....", ".....", "....." } },
  { '_', { ".....", ".....", ".....", ".....", ".....", ".....", "#####" } },
  { '+', { ".....", "..#..", "..#..", "#####", "..#..", "..#..", "....." } },
  { '=', { ".....", ".....", "#####", ".....", "#####", ".....", "....." } },
  { '%', { "##...", "##..#", "...#.", "..#..", ".#...", "#..##", "...##" } },
  { '#', { ".#.#.", ".#.#.", "#####", ".#.#.", "#####", ".#.#.", ".#.#." } },
  { '(', { "..#..", ".#...", "#....", "#....", "#....", ".#...", "..#.." } },
  { ')', { "..#..", "...#.", "....#", "....#", "....#", "...#.", "..#.." } },
  { '?', { ".###.", "#...#", "....#", "...#.", "..#..", ".....", "..#.." } },
  { '!', { "..#..", "..#..", "..#..", "..#..", "..#..", ".....", "..#.." } },
  { '<', { ".....", "...#.", "..#..", ".#...", "..#..", "...#.", "....." } },
  { '>', { ".....", ".#...", "..#..", "...#.", "..#..", ".#...", "....." } },
  { '*', { ".....", ".#.#.", "..#..", "#####", "..#..", ".#.#.", "....." } },
  { '[', { ".###.", ".#...", ".#...", ".#...", ".#...", ".#...", ".###." } },
  { ']', { ".###.", "...#.", "...#.", "...#.", "...#.", "...#.", ".###." } },
};

// Atlas grid: 16 cols × 8 rows of 6×8 cells.
constexpr int kCols = 16;
constexpr int kRows = 8;
constexpr int kAtlasW = kCols * TextRenderer::kCellW; // 96
constexpr int kAtlasH = kRows * TextRenderer::kCellH; // 64

void buildAtlasPixels(std::vector<uint8_t>& out) {
  out.assign(kAtlasW * kAtlasH, 0);
  for (const auto& g : kGlyphs) {
    unsigned char ch = static_cast<unsigned char>(g.c);
    if (ch >= 128) continue;
    int col = ch % kCols;
    int row = ch / kCols;
    int x0 = col * TextRenderer::kCellW;
    int y0 = row * TextRenderer::kCellH;
    for (int ry = 0; ry < TextRenderer::kCharH; ++ry) {
      const char* line = g.rows[ry];
      for (int rx = 0; rx < TextRenderer::kCharW; ++rx) {
        if (line[rx] == '#') {
          out[(y0 + ry) * kAtlasW + (x0 + rx)] = 255;
        }
      }
    }
  }
}

} // namespace

// ---------------------------------------------------------------------------
TextRenderer::~TextRenderer() {
  if (vbo_)      glDeleteBuffers(1, &vbo_);
  if (vao_)      glDeleteVertexArrays(1, &vao_);
  if (atlasTex_) glDeleteTextures(1, &atlasTex_);
  delete shader_;
}

bool TextRenderer::init() {
  shader_ = new Shader();
  if (!shader_->loadFromFiles("shaders/text.vert", "shaders/text.frag")) return false;

  std::vector<uint8_t> pixels;
  buildAtlasPixels(pixels);

  glGenTextures(1, &atlasTex_);
  glBindTexture(GL_TEXTURE_2D, atlasTex_);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, kAtlasW, kAtlasH, 0,
               GL_RED, GL_UNSIGNED_BYTE, pixels.data());
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glGenVertexArrays(1, &vao_);
  glGenBuffers(1, &vbo_);
  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  // pos(2) + uv(2) + color(3) = 7 floats, stride 28.
  const GLsizei stride = 7 * sizeof(float);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)(2*sizeof(float)));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (void*)(4*sizeof(float)));
  glBindVertexArray(0);
  return true;
}

void TextRenderer::draw(const char* text, int x, int y, float scale, Vec3 color) {
  if (!text) return;
  // Each glyph is kCharW × kCharH source pixels at `scale` pixels each.
  // Advance by (kCharW + 1) source pixels — small inter-character gap.
  float advance = (kCharW + 1) * scale;
  float gx = static_cast<float>(x);
  float gy = static_cast<float>(y);
  float w = kCharW * scale;
  float h = kCharH * scale;

  // UV size of one glyph cell in atlas space.
  const float uW = float(kCharW) / float(kAtlasW);
  const float vH = float(kCharH) / float(kAtlasH);

  for (const char* p = text; *p; ++p) {
    char c = *p;
    if (c == '\n') { gx = static_cast<float>(x); gy += h + 2.0f * scale; continue; }
    char up = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    int col = up % kCols;
    int row = up / kCols;
    float u0 = float(col * kCellW) / float(kAtlasW);
    float v0 = float(row * kCellH) / float(kAtlasH);
    float u1 = u0 + uW;
    float v1 = v0 + vH;

    // Pixel-space corners.
    float x0 = gx, y0 = gy;
    float x1 = gx + w, y1 = gy + h;
    // pos / uv / color per vertex (top-left origin in pixel space)
    float quad[6 * 7] = {
      x0, y0,  u0, v0,  color.x, color.y, color.z,
      x1, y0,  u1, v0,  color.x, color.y, color.z,
      x1, y1,  u1, v1,  color.x, color.y, color.z,
      x0, y0,  u0, v0,  color.x, color.y, color.z,
      x1, y1,  u1, v1,  color.x, color.y, color.z,
      x0, y1,  u0, v1,  color.x, color.y, color.z,
    };
    verts_.insert(verts_.end(), std::begin(quad), std::end(quad));
    gx += advance;
  }
}

void TextRenderer::drawf(int x, int y, float scale, Vec3 color, const char* fmt, ...) {
  char buf[256];
  va_list ap;
  va_start(ap, fmt);
  std::vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  draw(buf, x, y, scale, color);
}

void TextRenderer::reloadShaderIfChanged() {
  if (shader_) shader_->reloadIfChanged();
}

void TextRenderer::flush(int screenW, int screenH) {
  if (verts_.empty()) return;

  // Convert pixel-space x/y (top-left origin) → NDC in place. Vertex layout is
  // pos(2) + uv(2) + col(3) per vertex.
  for (size_t i = 0; i < verts_.size(); i += 7) {
    float px = verts_[i + 0];
    float py = verts_[i + 1];
    verts_[i + 0] =  (px / float(screenW)) * 2.0f - 1.0f;
    verts_[i + 1] = -(py / float(screenH)) * 2.0f + 1.0f;
  }

  glBindVertexArray(vao_);
  glBindBuffer(GL_ARRAY_BUFFER, vbo_);
  glBufferData(GL_ARRAY_BUFFER,
               static_cast<GLsizeiptr>(verts_.size() * sizeof(float)),
               verts_.data(), GL_DYNAMIC_DRAW);

  shader_->bind();
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, atlasTex_);
  shader_->setInt("uAtlas", 0);

  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  GLsizei vertCount = static_cast<GLsizei>(verts_.size() / 7);
  glDrawArrays(GL_TRIANGLES, 0, vertCount);

  glDisable(GL_BLEND);
  glBindVertexArray(0);
  verts_.clear();
}

} // namespace tyro
