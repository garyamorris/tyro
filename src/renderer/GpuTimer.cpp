#include "GpuTimer.h"
#include "gl_loader.h"

namespace tyro {

GpuTimer::~GpuTimer() { shutdown(); }

bool GpuTimer::init(const std::vector<std::string>& names) {
  shutdown();
  names_ = names;
  sections_.resize(names.size());
  for (auto& s : sections_) {
    glGenQueries(2, s.q);
  }
  cur_ = 0; prev_ = 1;
  active_ = true;
  return true;
}

void GpuTimer::shutdown() {
  if (!active_) return;
  for (auto& s : sections_) {
    if (s.q[0] || s.q[1]) glDeleteQueries(2, s.q);
    s.q[0] = s.q[1] = 0;
  }
  sections_.clear();
  names_.clear();
  active_ = false;
}

void GpuTimer::begin(int section) {
  if (!active_ || section < 0 || section >= (int)sections_.size()) return;
  glBeginQuery(GL_TIME_ELAPSED, sections_[section].q[cur_]);
}

void GpuTimer::end(int /*section*/) {
  if (!active_) return;
  glEndQuery(GL_TIME_ELAPSED);
}

void GpuTimer::endFrame() {
  if (!active_) return;
  // Read last frame's queries — likely complete by now.
  for (auto& s : sections_) {
    GLuint avail = 0;
    glGetQueryObjectuiv(s.q[prev_], GL_QUERY_RESULT_AVAILABLE, &avail);
    if (avail) {
      GLuint64 ns = 0;
      glGetQueryObjectui64v(s.q[prev_], GL_QUERY_RESULT, &ns);
      s.lastNs = ns;
    }
  }
  // Swap.
  int t = cur_; cur_ = prev_; prev_ = t;
}

double GpuTimer::resultMs(int section) const {
  if (section < 0 || section >= (int)sections_.size()) return 0.0;
  return double(sections_[section].lastNs) / 1.0e6;
}

} // namespace tyro
