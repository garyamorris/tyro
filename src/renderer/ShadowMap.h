#pragma once

namespace tyro {

// Depth-only FBO for directional-light shadow casting.
class ShadowMap {
public:
  ShadowMap() = default;
  ~ShadowMap();
  ShadowMap(const ShadowMap&) = delete;
  ShadowMap& operator=(const ShadowMap&) = delete;

  bool create(int size);
  void destroy();
  void bind() const;
  unsigned int depthTexture() const { return depthTex_; }
  int size() const { return size_; }

private:
  unsigned int fbo_      = 0;
  unsigned int depthTex_ = 0;
  int          size_     = 0;
};

} // namespace tyro
