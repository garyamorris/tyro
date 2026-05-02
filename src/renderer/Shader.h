#pragma once
#include <chrono>
#include <filesystem>
#include <string>

#include "math/Math.h"

namespace tyro {

class Shader {
public:
  Shader() = default;
  ~Shader();
  Shader(const Shader&) = delete;
  Shader& operator=(const Shader&) = delete;
  Shader(Shader&& o) noexcept { *this = static_cast<Shader&&>(o); }
  Shader& operator=(Shader&& o) noexcept;

  bool loadFromFiles(const char* vertPath, const char* fragPath, const char* geomPath = nullptr);
  bool loadFromSource(const char* vertSrc, const char* fragSrc, const char* geomSrc = nullptr);

  // Polls modtimes on the source files. If any changed, recompiles. On
  // failure, the existing program is preserved. Returns true iff the program
  // was successfully replaced.
  bool reloadIfChanged();

  void bind() const;
  unsigned int program() const { return program_; }

  void setInt  (const char* name, int v) const;
  void setFloat(const char* name, float v) const;
  void setVec2 (const char* name, Vec2 v) const;
  void setVec3 (const char* name, Vec3 v) const;
  void setMat3 (const char* name, const Mat3& m) const;
  void setMat4 (const char* name, const Mat4& m) const;

private:
  unsigned int program_ = 0;

  std::filesystem::path vertPath_, fragPath_, geomPath_;
  std::filesystem::file_time_type vertMTime_{}, fragMTime_{}, geomMTime_{};

  void destroy();
  static unsigned int compileAndLink(const char* vsrc, const char* fsrc, const char* gsrc);
};

} // namespace tyro
