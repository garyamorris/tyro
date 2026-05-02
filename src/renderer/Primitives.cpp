#include "Primitives.h"

#include <cmath>

namespace tyro {

void makeCube(std::vector<Vertex>& V, std::vector<uint32_t>& I) {
  V.clear(); I.clear();
  // 6 faces × 4 verts = 24 verts (split so each face has its own normal/uvs).
  struct F { Vec3 n; Vec3 corners[4]; };
  const F faces[6] = {
    { { 0, 0, 1}, { {-0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f} } },
    { { 0, 0,-1}, { { 0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f} } },
    { { 1, 0, 0}, { { 0.5f,-0.5f, 0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f, 0.5f,-0.5f}, { 0.5f, 0.5f, 0.5f} } },
    { {-1, 0, 0}, { {-0.5f,-0.5f,-0.5f}, {-0.5f,-0.5f, 0.5f}, {-0.5f, 0.5f, 0.5f}, {-0.5f, 0.5f,-0.5f} } },
    { { 0, 1, 0}, { {-0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f, 0.5f}, { 0.5f, 0.5f,-0.5f}, {-0.5f, 0.5f,-0.5f} } },
    { { 0,-1, 0}, { {-0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f,-0.5f}, { 0.5f,-0.5f, 0.5f}, {-0.5f,-0.5f, 0.5f} } },
  };
  Vec2 uvs[4] = { {0,0}, {1,0}, {1,1}, {0,1} };
  for (const auto& f : faces) {
    uint32_t base = static_cast<uint32_t>(V.size());
    for (int i = 0; i < 4; ++i) {
      V.push_back({ f.corners[i], f.n, uvs[i] });
    }
    I.push_back(base + 0); I.push_back(base + 1); I.push_back(base + 2);
    I.push_back(base + 0); I.push_back(base + 2); I.push_back(base + 3);
  }
}

void makeSphere(std::vector<Vertex>& V, std::vector<uint32_t>& I,
                int latSegs, int lonSegs, float radius) {
  V.clear(); I.clear();
  for (int y = 0; y <= latSegs; ++y) {
    float v = float(y) / float(latSegs);
    float theta = v * kPi;          // 0..pi
    float st = std::sin(theta), ct = std::cos(theta);
    for (int x = 0; x <= lonSegs; ++x) {
      float u = float(x) / float(lonSegs);
      float phi = u * kPi * 2.0f;   // 0..2pi
      float sp = std::sin(phi), cp = std::cos(phi);
      Vec3 n{ st*cp, ct, st*sp };
      V.push_back({ n * radius, n, Vec2{u, 1.0f - v} });
    }
  }
  int rowStride = lonSegs + 1;
  for (int y = 0; y < latSegs; ++y) {
    for (int x = 0; x < lonSegs; ++x) {
      uint32_t i0 = y * rowStride + x;
      uint32_t i1 = i0 + 1;
      uint32_t i2 = i0 + rowStride;
      uint32_t i3 = i2 + 1;
      I.push_back(i0); I.push_back(i2); I.push_back(i1);
      I.push_back(i1); I.push_back(i2); I.push_back(i3);
    }
  }
}

void makePlane(std::vector<Vertex>& V, std::vector<uint32_t>& I,
               float size, int subdivisions) {
  V.clear(); I.clear();
  if (subdivisions < 1) subdivisions = 1;
  int n = subdivisions + 1;
  float h = size * 0.5f;
  for (int z = 0; z < n; ++z) {
    for (int x = 0; x < n; ++x) {
      float u = float(x) / float(subdivisions);
      float v = float(z) / float(subdivisions);
      Vec3 p{ -h + u * size, 0.0f, -h + v * size };
      V.push_back({ p, Vec3{0,1,0}, Vec2{u, v} });
    }
  }
  for (int z = 0; z < subdivisions; ++z) {
    for (int x = 0; x < subdivisions; ++x) {
      uint32_t i0 = z * n + x;
      uint32_t i1 = i0 + 1;
      uint32_t i2 = i0 + n;
      uint32_t i3 = i2 + 1;
      I.push_back(i0); I.push_back(i2); I.push_back(i1);
      I.push_back(i1); I.push_back(i2); I.push_back(i3);
    }
  }
}

void makeTorus(std::vector<Vertex>& V, std::vector<uint32_t>& I,
               float majorR, float minorR, int majorSegs, int minorSegs) {
  V.clear(); I.clear();
  for (int j = 0; j <= majorSegs; ++j) {
    float u = float(j) / float(majorSegs);
    float a = u * 2.0f * kPi;
    float ca = std::cos(a), sa = std::sin(a);
    for (int i = 0; i <= minorSegs; ++i) {
      float v = float(i) / float(minorSegs);
      float b = v * 2.0f * kPi;
      float cb = std::cos(b), sb = std::sin(b);
      Vec3 center{ majorR * ca, 0.0f, majorR * sa };
      Vec3 dir{ ca * cb, sb, sa * cb };
      Vec3 p = center + dir * minorR;
      V.push_back({ p, normalize(dir), Vec2{u, v} });
    }
  }
  int rowStride = minorSegs + 1;
  for (int j = 0; j < majorSegs; ++j) {
    for (int i = 0; i < minorSegs; ++i) {
      uint32_t a = j * rowStride + i;
      uint32_t b = a + 1;
      uint32_t c = a + rowStride;
      uint32_t d = c + 1;
      I.push_back(a); I.push_back(c); I.push_back(b);
      I.push_back(b); I.push_back(c); I.push_back(d);
    }
  }
}

void makeCylinder(std::vector<Vertex>& V, std::vector<uint32_t>& I,
                  float radius, float height, int segs) {
  V.clear(); I.clear();
  float h = height * 0.5f;
  // Side
  for (int i = 0; i <= segs; ++i) {
    float u = float(i) / float(segs);
    float a = u * 2.0f * kPi;
    float c = std::cos(a), s = std::sin(a);
    Vec3 n{c, 0, s};
    V.push_back({ Vec3{c*radius, -h, s*radius}, n, Vec2{u, 0} });
    V.push_back({ Vec3{c*radius,  h, s*radius}, n, Vec2{u, 1} });
  }
  for (int i = 0; i < segs; ++i) {
    uint32_t a = i*2, b = a + 1, c = a + 2, d = a + 3;
    I.push_back(a); I.push_back(c); I.push_back(b);
    I.push_back(b); I.push_back(c); I.push_back(d);
  }
  // Top cap
  uint32_t topCenter = static_cast<uint32_t>(V.size());
  V.push_back({ Vec3{0, h, 0}, Vec3{0,1,0}, Vec2{0.5f, 0.5f} });
  uint32_t topRing0 = static_cast<uint32_t>(V.size());
  for (int i = 0; i <= segs; ++i) {
    float a = float(i) / float(segs) * 2.0f * kPi;
    float c = std::cos(a), s = std::sin(a);
    V.push_back({ Vec3{c*radius, h, s*radius}, Vec3{0,1,0},
                  Vec2{0.5f + 0.5f*c, 0.5f + 0.5f*s} });
  }
  for (int i = 0; i < segs; ++i) {
    I.push_back(topCenter);
    I.push_back(topRing0 + i);
    I.push_back(topRing0 + i + 1);
  }
  // Bottom cap
  uint32_t botCenter = static_cast<uint32_t>(V.size());
  V.push_back({ Vec3{0,-h, 0}, Vec3{0,-1,0}, Vec2{0.5f, 0.5f} });
  uint32_t botRing0 = static_cast<uint32_t>(V.size());
  for (int i = 0; i <= segs; ++i) {
    float a = float(i) / float(segs) * 2.0f * kPi;
    float c = std::cos(a), s = std::sin(a);
    V.push_back({ Vec3{c*radius, -h, s*radius}, Vec3{0,-1,0},
                  Vec2{0.5f + 0.5f*c, 0.5f - 0.5f*s} });
  }
  for (int i = 0; i < segs; ++i) {
    I.push_back(botCenter);
    I.push_back(botRing0 + i + 1);
    I.push_back(botRing0 + i);
  }
}

} // namespace tyro
