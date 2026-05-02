#include "ProceduralTextures.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace tyro {

namespace {

// ---- helpers --------------------------------------------------------------
constexpr float kPi = 3.14159265358979323846f;

inline std::uint32_t hashU32(std::uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352dU;
  x ^= x >> 15; x *= 0x846ca68bU;
  x ^= x >> 16;
  return x;
}
inline float hash01(int ix, int iy) {
  std::uint32_t h = hashU32(static_cast<std::uint32_t>(ix * 374761393)
                          ^ static_cast<std::uint32_t>(iy * 668265263));
  return float(h & 0x7FFFFFFFu) / float(0x7FFFFFFFu);
}
inline float smooth(float t) { return t * t * (3.0f - 2.0f * t); }
inline float lerp(float a, float b, float t) { return a + (b - a) * t; }

float valueNoise(float x, float y) {
  int ix = static_cast<int>(std::floor(x));
  int iy = static_cast<int>(std::floor(y));
  float fx = smooth(x - ix);
  float fy = smooth(y - iy);
  float a = hash01(ix,     iy);
  float b = hash01(ix + 1, iy);
  float c = hash01(ix,     iy + 1);
  float d = hash01(ix + 1, iy + 1);
  return lerp(lerp(a, b, fx), lerp(c, d, fx), fy);
}
float fbm(float x, float y, int octaves = 5) {
  float v = 0.0f, amp = 0.5f, freq = 1.0f;
  for (int i = 0; i < octaves; ++i) {
    v += amp * valueNoise(x * freq, y * freq);
    freq *= 2.0f; amp *= 0.5f;
  }
  return v;
}

inline std::uint8_t clampByte(float f) {
  if (f < 0.0f) f = 0.0f; if (f > 1.0f) f = 1.0f;
  return static_cast<std::uint8_t>(f * 255.0f + 0.5f);
}

void put(std::vector<std::uint8_t>& buf, int idx,
         float r, float g, float b, float a = 1.0f) {
  buf[idx + 0] = clampByte(r);
  buf[idx + 1] = clampByte(g);
  buf[idx + 2] = clampByte(b);
  buf[idx + 3] = clampByte(a);
}

} // namespace

// ---------------------------------------------------------------------------
std::vector<std::uint8_t> makeCheckerTex(int size, int cells) {
  std::vector<std::uint8_t> buf(size * size * 4);
  int cellSize = size / cells;
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      bool a = ((x / cellSize) + (y / cellSize)) & 1;
      float v = a ? 0.92f : 0.18f;
      put(buf, (y * size + x) * 4, v, v, v);
    }
  }
  return buf;
}

std::vector<std::uint8_t> makeBrickTex(int size) {
  std::vector<std::uint8_t> buf(size * size * 4);
  const int rowH    = size / 8;        // 8 rows of bricks
  const int brickW  = size / 4;        // 4 bricks per row
  const int mortar  = 3;
  for (int y = 0; y < size; ++y) {
    int row = y / rowH;
    int yIn = y % rowH;
    int offset = (row & 1) ? brickW / 2 : 0;
    for (int x = 0; x < size; ++x) {
      int xx = (x + offset) % size;
      int col = xx / brickW;
      int xIn = xx % brickW;

      bool isMortar = (yIn < mortar) || (xIn < mortar);
      float r, g, b;
      if (isMortar) {
        r = g = b = 0.32f + (hash01(x, y) - 0.5f) * 0.05f;
      } else {
        // Brick base color with per-brick variation + per-pixel grain.
        float baseR = 0.55f + 0.10f * hash01(col,     row);
        float baseG = 0.22f + 0.04f * hash01(col + 7, row + 3);
        float baseB = 0.16f + 0.04f * hash01(col + 1, row + 9);
        float grain = (valueNoise(x * 0.5f, y * 0.5f) - 0.5f) * 0.20f;
        r = baseR + grain;
        g = baseG + grain * 0.5f;
        b = baseB + grain * 0.4f;
      }
      put(buf, (y * size + x) * 4, r, g, b);
    }
  }
  return buf;
}

std::vector<std::uint8_t> makeWoodTex(int size) {
  std::vector<std::uint8_t> buf(size * size * 4);
  float cx = size * 0.5f, cy = size * 0.5f;
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      float dx = (x - cx) / size, dy = (y - cy) / size;
      float r = std::sqrt(dx*dx + dy*dy);
      // Distort radius by fbm to break the perfect rings.
      float distort = fbm(x * 0.02f, y * 0.02f, 4) - 0.5f;
      float ring = std::sin((r + distort * 0.4f) * 60.0f) * 0.5f + 0.5f;
      ring = std::pow(ring, 2.0f);
      // Mix between two browns.
      float t = ring;
      float R = lerp(0.42f, 0.62f, t);
      float G = lerp(0.24f, 0.42f, t);
      float B = lerp(0.10f, 0.20f, t);
      // Add a fine grain along Y.
      float grain = (valueNoise(x * 0.3f, y * 4.0f) - 0.5f) * 0.15f;
      R += grain; G += grain; B += grain;
      put(buf, (y * size + x) * 4, R, G, B);
    }
  }
  return buf;
}

std::vector<std::uint8_t> makeMarbleTex(int size) {
  std::vector<std::uint8_t> buf(size * size * 4);
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      float fx = float(x) / float(size);
      float fy = float(y) / float(size);
      // Turbulence-modulated sin band.
      float t = std::sin(fx * 8.0f + fbm(fx * 6.0f, fy * 6.0f, 6) * 6.0f);
      t = 0.5f + 0.5f * t;
      t = std::pow(t, 5.0f);
      float R = lerp(0.78f, 0.98f, t);
      float G = lerp(0.78f, 0.98f, t);
      float B = lerp(0.84f, 1.00f, t);
      // Faint blue veins.
      float vein = std::pow(t, 30.0f) * 0.4f;
      R -= vein; G -= vein * 0.8f;
      put(buf, (y * size + x) * 4, R, G, B);
    }
  }
  return buf;
}

std::vector<std::uint8_t> makeNoiseTex(int size) {
  std::vector<std::uint8_t> buf(size * size * 4);
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      float n = fbm(x * 0.04f, y * 0.04f, 5);
      put(buf, (y * size + x) * 4, n, n, n);
    }
  }
  return buf;
}

std::vector<std::uint8_t> makeHexTex(int size) {
  std::vector<std::uint8_t> buf(size * size * 4);
  // Cellular hex pattern: distance to nearest hex center.
  const float hexR = size / 12.0f;
  const float vertSpacing = hexR * std::sqrt(3.0f);
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      float fx = float(x), fy = float(y);

      // Find nearest hex center using the simple "two grids" offset trick.
      float bestDist = 1e9f;
      for (int oy = -1; oy <= 1; ++oy) {
        for (int ox = -1; ox <= 1; ++ox) {
          int row = static_cast<int>(std::floor(fy / vertSpacing)) + oy;
          float cy = row * vertSpacing;
          float colShift = (row & 1) ? hexR * 1.5f : 0.0f;
          int col = static_cast<int>(std::floor((fx - colShift) / (hexR * 3.0f))) + ox;
          float cx = col * hexR * 3.0f + colShift;
          float dx = fx - cx, dy = fy - cy;
          float d = std::sqrt(dx*dx + dy*dy);
          if (d < bestDist) bestDist = d;
        }
      }

      // Make a thick border + soft interior.
      float t = bestDist / hexR;
      float border = (t > 0.85f) ? 1.0f : 0.0f;
      float interior = std::max(0.0f, 1.0f - t * 1.3f);

      float R = border * 0.05f + interior * 0.30f;
      float G = border * 0.05f + interior * 0.85f;
      float B = border * 0.10f + interior * 0.70f;
      put(buf, (y * size + x) * 4, R, G, B);
    }
  }
  return buf;
}

std::vector<std::uint8_t> makeRoughNormalMap(int size, float strength) {
  std::vector<std::uint8_t> buf(size * size * 4);
  // Heightmap is fbm sampled at the texel; sample wrapped neighbours to
  // produce a tileable normal map.
  auto sampleH = [&](int x, int y) {
    x = ((x % size) + size) % size;
    y = ((y % size) + size) % size;
    return fbm(x * 0.04f, y * 0.04f, 5);
  };
  for (int y = 0; y < size; ++y) {
    for (int x = 0; x < size; ++x) {
      // Central differences -> tangent-space gradient.
      float dx = (sampleH(x + 1, y) - sampleH(x - 1, y)) * strength;
      float dy = (sampleH(x, y + 1) - sampleH(x, y - 1)) * strength;
      // Normal in tangent space: (-dx, -dy, 1) normalised.
      float nx = -dx, ny = -dy, nz = 1.0f;
      float len = std::sqrt(nx*nx + ny*ny + nz*nz);
      if (len < 1e-8f) len = 1.0f;
      nx /= len; ny /= len; nz /= len;
      // [-1,1] -> [0,1] -> byte
      put(buf, (y * size + x) * 4,
          nx * 0.5f + 0.5f,
          ny * 0.5f + 0.5f,
          nz * 0.5f + 0.5f);
    }
  }
  return buf;
}

} // namespace tyro
