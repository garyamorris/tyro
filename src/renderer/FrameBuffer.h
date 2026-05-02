#pragma once

namespace tyro {

enum class DepthMode {
  None,           // no depth attachment (post-process ping-pong FBO)
  Renderbuffer,   // write-only depth (cheapest)
  Texture         // sampleable depth texture (depth fog, SSAO)
};

class FrameBuffer {
public:
  FrameBuffer() = default;
  ~FrameBuffer();
  FrameBuffer(const FrameBuffer&) = delete;
  FrameBuffer& operator=(const FrameBuffer&) = delete;

  // `hdr` chooses RGBA16F over RGBA8 for the color attachment — needed when
  // the FBO must hold linear HDR values (PBR + IBL + bloom intermediate).
  bool create(int width, int height,
              DepthMode depth = DepthMode::Renderbuffer,
              bool hdr = false);
  void resize(int width, int height);
  void destroy();

  void bind() const;
  static void bindDefault();

  unsigned int colorTexture() const { return colorTex_; }
  unsigned int depthTexture() const { return depthTex_; } // 0 unless DepthMode::Texture
  int width()  const { return width_; }
  int height() const { return height_; }

private:
  unsigned int fbo_      = 0;
  unsigned int colorTex_ = 0;
  unsigned int depthTex_ = 0; // when depthMode_ == Texture
  unsigned int depthRbo_ = 0; // when depthMode_ == Renderbuffer
  DepthMode    depthMode_ = DepthMode::Renderbuffer;
  bool         hdr_      = false;
  int width_ = 0, height_ = 0;
};

} // namespace tyro
