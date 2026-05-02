// tyro math: hand-rolled, column-major (matches OpenGL/GLSL).
#pragma once

#include <cmath>
#include <cstring>
#include <cstddef>

namespace tyro {

constexpr float kPi = 3.14159265358979323846f;
inline float radians(float deg) { return deg * (kPi / 180.0f); }
inline float degrees(float rad) { return rad * (180.0f / kPi); }

// ---------------- Vec2 ----------------
struct Vec2 {
  float x = 0, y = 0;
  Vec2() = default;
  Vec2(float xx, float yy) : x(xx), y(yy) {}
  float& operator[](int i)       { return (&x)[i]; }
  float  operator[](int i) const { return (&x)[i]; }
};
inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x+b.x, a.y+b.y}; }
inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x-b.x, a.y-b.y}; }
inline Vec2 operator*(Vec2 a, float s) { return {a.x*s, a.y*s}; }

// ---------------- Vec3 ----------------
struct Vec3 {
  float x = 0, y = 0, z = 0;
  Vec3() = default;
  Vec3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}
  explicit Vec3(float v) : x(v), y(v), z(v) {}
  float& operator[](int i)       { return (&x)[i]; }
  float  operator[](int i) const { return (&x)[i]; }
};
inline Vec3 operator+(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3 operator-(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline Vec3 operator-(Vec3 a)         { return {-a.x, -a.y, -a.z}; }
inline Vec3 operator*(Vec3 a, float s){ return {a.x*s, a.y*s, a.z*s}; }
inline Vec3 operator*(float s, Vec3 a){ return a * s; }
inline Vec3 operator*(Vec3 a, Vec3 b) { return {a.x*b.x, a.y*b.y, a.z*b.z}; }
inline Vec3 operator/(Vec3 a, float s){ return {a.x/s, a.y/s, a.z/s}; }
inline Vec3& operator+=(Vec3& a, Vec3 b){ a = a + b; return a; }

inline float dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3  cross(Vec3 a, Vec3 b) {
  return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
inline float length(Vec3 a) { return std::sqrt(dot(a, a)); }
inline Vec3  normalize(Vec3 a) {
  float L = length(a);
  return L > 0.0f ? a / L : Vec3{0,0,0};
}

// ---------------- Vec4 ----------------
struct Vec4 {
  float x = 0, y = 0, z = 0, w = 0;
  Vec4() = default;
  Vec4(float xx, float yy, float zz, float ww) : x(xx), y(yy), z(zz), w(ww) {}
  Vec4(Vec3 v, float ww) : x(v.x), y(v.y), z(v.z), w(ww) {}
  float& operator[](int i)       { return (&x)[i]; }
  float  operator[](int i) const { return (&x)[i]; }
};
inline Vec4 operator+(Vec4 a, Vec4 b) { return {a.x+b.x, a.y+b.y, a.z+b.z, a.w+b.w}; }
inline Vec4 operator*(Vec4 a, float s){ return {a.x*s, a.y*s, a.z*s, a.w*s}; }

// ---------------- Mat4 (column-major) ----------------
// m[col*4 + row]. Storage layout matches GL: column 0 is m[0..3].
struct Mat4 {
  float m[16];

  Mat4() { for (int i = 0; i < 16; ++i) m[i] = 0.0f; }

  static Mat4 identity() {
    Mat4 r;
    r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f;
    return r;
  }

  // r = row, c = column.
  float&  at(int r, int c)       { return m[c*4 + r]; }
  float   at(int r, int c) const { return m[c*4 + r]; }

  const float* data() const { return m; }
};

inline Mat4 operator*(const Mat4& A, const Mat4& B) {
  Mat4 R;
  for (int c = 0; c < 4; ++c) {
    for (int r = 0; r < 4; ++r) {
      float s = 0.0f;
      for (int k = 0; k < 4; ++k) s += A.at(r, k) * B.at(k, c);
      R.at(r, c) = s;
    }
  }
  return R;
}

inline Vec4 operator*(const Mat4& A, Vec4 v) {
  Vec4 r;
  for (int row = 0; row < 4; ++row) {
    r[row] = A.at(row, 0)*v.x + A.at(row, 1)*v.y + A.at(row, 2)*v.z + A.at(row, 3)*v.w;
  }
  return r;
}

inline Mat4 translate(Vec3 t) {
  Mat4 r = Mat4::identity();
  r.at(0, 3) = t.x; r.at(1, 3) = t.y; r.at(2, 3) = t.z;
  return r;
}

inline Mat4 scale(Vec3 s) {
  Mat4 r;
  r.at(0,0) = s.x; r.at(1,1) = s.y; r.at(2,2) = s.z; r.at(3,3) = 1.0f;
  return r;
}

// Right-handed rotation about a normalized axis.
inline Mat4 rotate(float angleRad, Vec3 axis) {
  Vec3 a = normalize(axis);
  float c = std::cos(angleRad), s = std::sin(angleRad), ic = 1.0f - c;
  Mat4 r = Mat4::identity();
  r.at(0,0) = c + a.x*a.x*ic;
  r.at(1,0) = a.y*a.x*ic + a.z*s;
  r.at(2,0) = a.z*a.x*ic - a.y*s;

  r.at(0,1) = a.x*a.y*ic - a.z*s;
  r.at(1,1) = c + a.y*a.y*ic;
  r.at(2,1) = a.z*a.y*ic + a.x*s;

  r.at(0,2) = a.x*a.z*ic + a.y*s;
  r.at(1,2) = a.y*a.z*ic - a.x*s;
  r.at(2,2) = c + a.z*a.z*ic;
  return r;
}

// Right-handed orthographic, depth range [-1, 1] (GL convention).
inline Mat4 ortho(float l, float r, float b, float t, float zNear, float zFar) {
  Mat4 m = Mat4::identity();
  m.at(0,0) =  2.0f / (r - l);
  m.at(1,1) =  2.0f / (t - b);
  m.at(2,2) = -2.0f / (zFar - zNear);
  m.at(0,3) = -(r + l) / (r - l);
  m.at(1,3) = -(t + b) / (t - b);
  m.at(2,3) = -(zFar + zNear) / (zFar - zNear);
  return m;
}

// Right-handed perspective, depth range [-1, 1] (GL convention).
inline Mat4 perspective(float fovYRad, float aspect, float zNear, float zFar) {
  float f = 1.0f / std::tan(fovYRad * 0.5f);
  Mat4 r;
  r.at(0,0) = f / aspect;
  r.at(1,1) = f;
  r.at(2,2) = (zFar + zNear) / (zNear - zFar);
  r.at(2,3) = (2.0f * zFar * zNear) / (zNear - zFar);
  r.at(3,2) = -1.0f;
  return r;
}

inline Mat4 lookAt(Vec3 eye, Vec3 target, Vec3 up) {
  Vec3 f = normalize(target - eye);
  Vec3 s = normalize(cross(f, up));
  Vec3 u = cross(s, f);
  Mat4 r = Mat4::identity();
  r.at(0,0) =  s.x; r.at(0,1) =  s.y; r.at(0,2) =  s.z;
  r.at(1,0) =  u.x; r.at(1,1) =  u.y; r.at(1,2) =  u.z;
  r.at(2,0) = -f.x; r.at(2,1) = -f.y; r.at(2,2) = -f.z;
  r.at(0,3) = -dot(s, eye);
  r.at(1,3) = -dot(u, eye);
  r.at(2,3) =  dot(f, eye);
  return r;
}

inline Mat4 transpose(const Mat4& A) {
  Mat4 R;
  for (int r = 0; r < 4; ++r)
    for (int c = 0; c < 4; ++c) R.at(r, c) = A.at(c, r);
  return R;
}

// 3x3 inverse-transpose of upper-left of a 4x4 — for transforming normals.
struct Mat3 {
  float m[9]; // column-major
  static Mat3 fromMat4Upper(const Mat4& M) {
    Mat3 r;
    for (int c = 0; c < 3; ++c)
      for (int rw = 0; rw < 3; ++rw)
        r.m[c*3 + rw] = M.at(rw, c);
    return r;
  }
  const float* data() const { return m; }
};

inline Mat3 inverseTranspose(const Mat3& A) {
  // Compute classical adjugate / determinant on column-major 3x3.
  auto a = [&](int r, int c) { return A.m[c*3 + r]; };
  float c00 =  (a(1,1)*a(2,2) - a(1,2)*a(2,1));
  float c01 = -(a(1,0)*a(2,2) - a(1,2)*a(2,0));
  float c02 =  (a(1,0)*a(2,1) - a(1,1)*a(2,0));
  float c10 = -(a(0,1)*a(2,2) - a(0,2)*a(2,1));
  float c11 =  (a(0,0)*a(2,2) - a(0,2)*a(2,0));
  float c12 = -(a(0,0)*a(2,1) - a(0,1)*a(2,0));
  float c20 =  (a(0,1)*a(1,2) - a(0,2)*a(1,1));
  float c21 = -(a(0,0)*a(1,2) - a(0,2)*a(1,0));
  float c22 =  (a(0,0)*a(1,1) - a(0,1)*a(1,0));

  float det = a(0,0)*c00 + a(0,1)*c01 + a(0,2)*c02;
  Mat3 R;
  if (std::abs(det) < 1e-8f) { for (int i = 0; i < 9; ++i) R.m[i] = (i%4==0)?1.0f:0.0f; return R; }
  float inv = 1.0f / det;
  // inverse-transpose == cofactor matrix / det, stored column-major.
  R.m[0*3+0] = c00 * inv; R.m[0*3+1] = c01 * inv; R.m[0*3+2] = c02 * inv;
  R.m[1*3+0] = c10 * inv; R.m[1*3+1] = c11 * inv; R.m[1*3+2] = c12 * inv;
  R.m[2*3+0] = c20 * inv; R.m[2*3+1] = c21 * inv; R.m[2*3+2] = c22 * inv;
  return R;
}

// ---------------- Quaternion ----------------
struct Quat {
  float x = 0, y = 0, z = 0, w = 1;
  static Quat identity() { return {0,0,0,1}; }
  static Quat fromAxisAngle(Vec3 axis, float angleRad) {
    Vec3 a = normalize(axis);
    float h = angleRad * 0.5f;
    float s = std::sin(h);
    return { a.x*s, a.y*s, a.z*s, std::cos(h) };
  }
};
inline Quat operator*(Quat a, Quat b) {
  return {
    a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
    a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
    a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
    a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z,
  };
}
inline Mat4 toMat4(Quat q) {
  float xx = q.x*q.x, yy = q.y*q.y, zz = q.z*q.z;
  float xy = q.x*q.y, xz = q.x*q.z, yz = q.y*q.z;
  float wx = q.w*q.x, wy = q.w*q.y, wz = q.w*q.z;
  Mat4 r = Mat4::identity();
  r.at(0,0) = 1 - 2*(yy+zz); r.at(0,1) = 2*(xy-wz);     r.at(0,2) = 2*(xz+wy);
  r.at(1,0) = 2*(xy+wz);     r.at(1,1) = 1 - 2*(xx+zz); r.at(1,2) = 2*(yz-wx);
  r.at(2,0) = 2*(xz-wy);     r.at(2,1) = 2*(yz+wx);     r.at(2,2) = 1 - 2*(xx+yy);
  return r;
}

} // namespace tyro
