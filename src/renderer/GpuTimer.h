#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace tyro {

// Timestamp-based GPU timer. Uses GL_TIME_ELAPSED queries with a 2-frame
// ping-pong so the GPU is never stalled waiting for a result — we read the
// previous frame's queries at endFrame().
class GpuTimer {
public:
  GpuTimer() = default;
  ~GpuTimer();
  GpuTimer(const GpuTimer&) = delete;
  GpuTimer& operator=(const GpuTimer&) = delete;

  bool init(const std::vector<std::string>& sectionNames);
  void shutdown();

  void begin(int section);
  void end  (int section);
  // Reads previous frame's results then swaps which buffer is "current".
  void endFrame();

  double resultMs(int section) const;
  int    sectionCount() const { return static_cast<int>(names_.size()); }
  const std::string& name(int i) const { return names_[i]; }

private:
  struct Section {
    unsigned int q[2] = {0, 0};
    uint64_t     lastNs = 0;
  };
  std::vector<Section> sections_;
  std::vector<std::string> names_;
  int cur_  = 0;
  int prev_ = 1;
  bool active_ = false;
};

} // namespace tyro
