#include "Shader.h"
#include "gl_loader.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <vector>

namespace fs = std::filesystem;

namespace tyro {

static bool readFile(const char* path, std::string& out) {
  std::ifstream f(path, std::ios::binary);
  if (!f) { std::fprintf(stderr, "[shader] cannot open %s\n", path); return false; }
  std::ostringstream ss; ss << f.rdbuf();
  out = ss.str();
  return true;
}

static unsigned int compileStage(unsigned int kind, const char* src, const char* tag) {
  unsigned int sh = glCreateShader(kind);
  glShaderSource(sh, 1, &src, nullptr);
  glCompileShader(sh);
  GLint ok = 0;
  glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    GLint len = 0;
    glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
    std::vector<char> log(static_cast<size_t>(len > 0 ? len : 1));
    glGetShaderInfoLog(sh, len, nullptr, log.data());
    std::fprintf(stderr, "[shader] %s compile error:\n%s\n", tag, log.data());
    glDeleteShader(sh);
    return 0;
  }
  return sh;
}

unsigned int Shader::compileAndLink(const char* vertSrc, const char* fragSrc, const char* geomSrc) {
  unsigned int v = compileStage(GL_VERTEX_SHADER,   vertSrc, "vertex");
  if (!v) return 0;
  unsigned int f = compileStage(GL_FRAGMENT_SHADER, fragSrc, "fragment");
  if (!f) { glDeleteShader(v); return 0; }
  unsigned int g = 0;
  if (geomSrc) {
    g = compileStage(GL_GEOMETRY_SHADER, geomSrc, "geometry");
    if (!g) { glDeleteShader(v); glDeleteShader(f); return 0; }
  }

  unsigned int p = glCreateProgram();
  glAttachShader(p, v);
  glAttachShader(p, f);
  if (g) glAttachShader(p, g);
  glLinkProgram(p);
  glDeleteShader(v);
  glDeleteShader(f);
  if (g) glDeleteShader(g);

  GLint ok = 0;
  glGetProgramiv(p, GL_LINK_STATUS, &ok);
  if (!ok) {
    GLint len = 0;
    glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
    std::vector<char> log(static_cast<size_t>(len > 0 ? len : 1));
    glGetProgramInfoLog(p, len, nullptr, log.data());
    std::fprintf(stderr, "[shader] link error:\n%s\n", log.data());
    glDeleteProgram(p);
    return 0;
  }
  return p;
}

Shader& Shader::operator=(Shader&& o) noexcept {
  if (this != &o) {
    destroy();
    program_ = o.program_; o.program_ = 0;
    vertPath_ = std::move(o.vertPath_); fragPath_ = std::move(o.fragPath_); geomPath_ = std::move(o.geomPath_);
    vertMTime_ = o.vertMTime_; fragMTime_ = o.fragMTime_; geomMTime_ = o.geomMTime_;
  }
  return *this;
}
Shader::~Shader() { destroy(); }
void Shader::destroy() { if (program_) { glDeleteProgram(program_); program_ = 0; } }

bool Shader::loadFromFiles(const char* vertPath, const char* fragPath, const char* geomPath) {
  std::string vs, fs, gs;
  if (!readFile(vertPath, vs)) return false;
  if (!readFile(fragPath, fs)) return false;
  if (geomPath && !readFile(geomPath, gs)) return false;
  if (!loadFromSource(vs.c_str(), fs.c_str(), geomPath ? gs.c_str() : nullptr)) return false;

  // Cache paths + modtimes for hot-reload.
  std::error_code ec;
  vertPath_ = vertPath;
  fragPath_ = fragPath;
  geomPath_ = geomPath ? std::filesystem::path{geomPath} : std::filesystem::path{};
  vertMTime_ = fs::last_write_time(vertPath_, ec);
  fragMTime_ = fs::last_write_time(fragPath_, ec);
  if (!geomPath_.empty()) geomMTime_ = fs::last_write_time(geomPath_, ec);
  return true;
}

bool Shader::loadFromSource(const char* vertSrc, const char* fragSrc, const char* geomSrc) {
  unsigned int p = compileAndLink(vertSrc, fragSrc, geomSrc);
  if (!p) return false;
  destroy();
  program_ = p;
  return true;
}

bool Shader::reloadIfChanged() {
  if (vertPath_.empty() || fragPath_.empty()) return false;

  std::error_code ec;
  auto vt = fs::last_write_time(vertPath_, ec); if (ec) return false;
  auto ft = fs::last_write_time(fragPath_, ec); if (ec) return false;
  fs::file_time_type gt{};
  if (!geomPath_.empty()) {
    gt = fs::last_write_time(geomPath_, ec);
    if (ec) return false;
  }

  bool changed = (vt != vertMTime_) || (ft != fragMTime_) ||
                 (!geomPath_.empty() && gt != geomMTime_);
  if (!changed) return false;

  // Update timestamps regardless — if compile fails we don't want to spin on
  // the same broken file every frame. The user will save again to retry.
  vertMTime_ = vt; fragMTime_ = ft; geomMTime_ = gt;

  std::string vs, fs, gs;
  if (!readFile(vertPath_.string().c_str(), vs)) return false;
  if (!readFile(fragPath_.string().c_str(), fs)) return false;
  if (!geomPath_.empty() && !readFile(geomPath_.string().c_str(), gs)) return false;

  unsigned int p = compileAndLink(vs.c_str(), fs.c_str(),
                                  geomPath_.empty() ? nullptr : gs.c_str());
  if (!p) {
    std::fprintf(stderr, "[shader] reload FAILED for %s — keeping old program\n",
                 fragPath_.filename().string().c_str());
    return false;
  }
  destroy();
  program_ = p;
  std::fprintf(stderr, "[shader] reloaded %s\n", fragPath_.filename().string().c_str());
  return true;
}

void Shader::bind() const { glUseProgram(program_); }

void Shader::setInt(const char* n, int v) const {
  glUniform1i(glGetUniformLocation(program_, n), v);
}
void Shader::setFloat(const char* n, float v) const {
  glUniform1f(glGetUniformLocation(program_, n), v);
}
void Shader::setVec2(const char* n, Vec2 v) const {
  glUniform2f(glGetUniformLocation(program_, n), v.x, v.y);
}
void Shader::setVec3(const char* n, Vec3 v) const {
  glUniform3f(glGetUniformLocation(program_, n), v.x, v.y, v.z);
}
void Shader::setMat3(const char* n, const Mat3& m) const {
  glUniformMatrix3fv(glGetUniformLocation(program_, n), 1, GL_FALSE, m.data());
}
void Shader::setMat4(const char* n, const Mat4& m) const {
  glUniformMatrix4fv(glGetUniformLocation(program_, n), 1, GL_FALSE, m.data());
}

} // namespace tyro
