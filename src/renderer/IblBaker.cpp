#include "IblBaker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "Shader.h"
#include "gl_loader.h"
#include "stb_image.h"

namespace tyro {

namespace {

struct CaptureViews {
  Mat4 proj;
  Mat4 views[6];
};

CaptureViews makeCaptureViews() {
  CaptureViews cv;
  cv.proj = perspective(kPi * 0.5f, 1.0f, 0.1f, 10.0f);
  // GL cubemap convention is left-handed inside the cube; flipped up-vectors
  // are the canonical learnopengl set. Order matches POSITIVE_X..NEGATIVE_Z.
  Vec3 z{0,0,0};
  cv.views[0] = lookAt(z, Vec3{ 1, 0, 0}, Vec3{0,-1, 0}); // +X
  cv.views[1] = lookAt(z, Vec3{-1, 0, 0}, Vec3{0,-1, 0}); // -X
  cv.views[2] = lookAt(z, Vec3{ 0, 1, 0}, Vec3{0, 0, 1}); // +Y
  cv.views[3] = lookAt(z, Vec3{ 0,-1, 0}, Vec3{0, 0,-1}); // -Y
  cv.views[4] = lookAt(z, Vec3{ 0, 0, 1}, Vec3{0,-1, 0}); // +Z
  cv.views[5] = lookAt(z, Vec3{ 0, 0,-1}, Vec3{0,-1, 0}); // -Z
  return cv;
}

// Position-only unit cube (36 verts). Used for cube-capture passes and the
// skybox.
const float kCubeVerts[] = {
  -1,-1,-1,  1, 1,-1,  1,-1,-1,
   1, 1,-1, -1,-1,-1, -1, 1,-1,
  -1,-1, 1,  1,-1, 1,  1, 1, 1,
   1, 1, 1, -1, 1, 1, -1,-1, 1,
  -1, 1, 1, -1, 1,-1, -1,-1,-1,
  -1,-1,-1, -1,-1, 1, -1, 1, 1,
   1, 1, 1,  1,-1,-1,  1, 1,-1,
   1,-1,-1,  1, 1, 1,  1,-1, 1,
  -1,-1,-1,  1,-1,-1,  1,-1, 1,
   1,-1, 1, -1,-1, 1, -1,-1,-1,
  -1, 1,-1,  1, 1, 1,  1, 1,-1,
   1, 1, 1, -1, 1,-1, -1, 1, 1,
};

bool checkFbo(const char* tag) {
  GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
  if (st != GL_FRAMEBUFFER_COMPLETE) {
    std::fprintf(stderr, "[ibl] %s: framebuffer incomplete (0x%X)\n", tag, st);
    return false;
  }
  return true;
}

} // namespace

// ---------------------------------------------------------------------------
// Procedural HDR sky.
//
// Hand-built rather than Hosek-Wilkie:
//   - smooth gradient from warm horizon to deep zenith blue
//   - bright HDR sun disc (peak ~80x for nice bloom + IBL highlights)
//   - dim ground tint below the horizon
// Output is RGB float, equirectangular layout (X=phi, Y=theta).
std::vector<float> makeProceduralSkyHDR(int width, int height,
                                        Vec3 sunDir, float sunIntensity) {
  std::vector<float> data(static_cast<size_t>(width) * height * 3);
  Vec3 S = normalize(sunDir);
  for (int y = 0; y < height; ++y) {
    float theta = (float(y) + 0.5f) / float(height) * kPi;
    float ct = std::cos(theta);
    float st = std::sin(theta);
    for (int x = 0; x < width; ++x) {
      float phi = ((float(x) + 0.5f) / float(width)) * 2.0f * kPi - kPi;
      float cp  = std::cos(phi), sp = std::sin(phi);
      Vec3 dir{ st * cp, ct, st * sp };

      float upT = std::max(0.0f, dir.y);
      Vec3 zenith {0.18f, 0.30f, 0.55f};
      Vec3 horizon{0.85f, 0.78f, 0.65f};
      Vec3 ground {0.18f, 0.16f, 0.14f};
      Vec3 col;
      if (dir.y >= 0.0f) {
        float t = std::pow(upT, 0.6f);
        col = horizon + (zenith - horizon) * t;
      } else {
        float t = std::pow(-dir.y, 0.5f);
        col = horizon + (ground - horizon) * t;
      }

      float cosToSun = std::max(-1.0f, std::min(1.0f, dot(dir, S)));
      float halo = std::pow(std::max(0.0f, cosToSun), 8.0f);
      col = col + Vec3{1.0f, 0.55f, 0.20f} * (halo * 0.7f);

      float angle = std::acos(cosToSun);
      const float sunRadius = 0.045f;
      float disc = 1.0f - std::min(1.0f, angle / sunRadius);
      disc = disc * disc;
      col = col + Vec3{1.0f, 0.95f, 0.85f} * (disc * sunIntensity);

      size_t idx = (static_cast<size_t>(y) * width + x) * 3;
      data[idx + 0] = col.x;
      data[idx + 1] = col.y;
      data[idx + 2] = col.z;
    }
  }
  return data;
}

std::vector<float> loadEquirectHDRFromFile(const char* path,
                                           int& outW, int& outH) {
  // HDR equirect files are conventionally stored top-row-first (top of the
  // sphere at y=0), matching makeProceduralSkyHDR's indexing. Disable the
  // global flag stbi might have inherited from Texture's PNG/JPG loads.
  stbi_set_flip_vertically_on_load(0);

  int w = 0, h = 0, c = 0;
  float* px = stbi_loadf(path, &w, &h, &c, 3); // force 3-channel RGB
  if (!px) {
    return {};
  }
  std::vector<float> out(px, px + static_cast<size_t>(w) * h * 3);
  stbi_image_free(px);
  outW = w;
  outH = h;
  return out;
}

unsigned int uploadEquirectHDR(const float* rgbF, int width, int height) {
  GLuint tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0,
               GL_RGB, GL_FLOAT, rgbF);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  return tex;
}

// ---------------------------------------------------------------------------
IblBaker::~IblBaker() { destroy(); }

void IblBaker::destroy() {
  env_.destroy();
  irradiance_.destroy();
  prefilter_.destroy();
  if (brdfLut_)    { glDeleteTextures(1, &brdfLut_); brdfLut_ = 0; }
  if (cubeVbo_)    { glDeleteBuffers(1, &cubeVbo_); cubeVbo_ = 0; }
  if (cubeVao_)    { glDeleteVertexArrays(1, &cubeVao_); cubeVao_ = 0; }
  if (captureFbo_) { glDeleteFramebuffers(1, &captureFbo_); captureFbo_ = 0; }
  if (captureRbo_) { glDeleteRenderbuffers(1, &captureRbo_); captureRbo_ = 0; }
}

void IblBaker::initCaptureGeometry() {
  if (cubeVao_) return;
  glGenVertexArrays(1, &cubeVao_);
  glBindVertexArray(cubeVao_);
  glGenBuffers(1, &cubeVbo_);
  glBindBuffer(GL_ARRAY_BUFFER, cubeVbo_);
  glBufferData(GL_ARRAY_BUFFER, sizeof(kCubeVerts), kCubeVerts, GL_STATIC_DRAW);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  glBindVertexArray(0);
}

void IblBaker::drawUnitCube() const {
  glBindVertexArray(cubeVao_);
  glDrawArrays(GL_TRIANGLES, 0, 36);
}

bool IblBaker::bakeFromEquirect(unsigned int equirectTex, int srcW, int srcH) {
  (void)srcW; (void)srcH;
  initCaptureGeometry();

  if (!captureFbo_) glGenFramebuffers(1, &captureFbo_);
  if (!captureRbo_) glGenRenderbuffers(1, &captureRbo_);

  auto sizeDepth = [&](int s) {
    glBindRenderbuffer(GL_RENDERBUFFER, captureRbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, s, s);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFbo_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
                              GL_RENDERBUFFER, captureRbo_);
  };

  Shader shEquirect, shIrradiance, shPrefilter, shBrdfLut;
  if (!shEquirect  .loadFromFiles("shaders/cubemap_capture.vert", "shaders/equirect_to_cube.frag")) return false;
  if (!shIrradiance.loadFromFiles("shaders/cubemap_capture.vert", "shaders/irradiance_conv.frag"))  return false;
  if (!shPrefilter .loadFromFiles("shaders/cubemap_capture.vert", "shaders/prefilter.frag"))        return false;
  if (!shBrdfLut   .loadFromFiles("shaders/blit.vert",            "shaders/brdf_lut.frag"))         return false;

  CaptureViews cv = makeCaptureViews();

  GLint prevViewport[4]; glGetIntegerv(0x0BA2 /* GL_VIEWPORT */, prevViewport);
  glDisable(GL_CULL_FACE);
  glDepthFunc(GL_LESS);
  glEnable(GL_DEPTH_TEST);

  // ----- 1. Equirect -> environment cubemap (allocate full mip chain) -----
  const int kEnvFace = 512;
  const int envMips = static_cast<int>(std::floor(std::log2(float(kEnvFace)))) + 1;
  if (!env_.create(kEnvFace, GL_RGB16F, GL_RGB, GL_FLOAT, envMips, true)) return false;

  sizeDepth(kEnvFace);
  shEquirect.bind();
  shEquirect.setInt("uEquirect", 0);
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, equirectTex);

  glBindFramebuffer(GL_FRAMEBUFFER, captureFbo_);
  glViewport(0, 0, kEnvFace, kEnvFace);
  shEquirect.setMat4("uProj", cv.proj);
  for (int f = 0; f < 6; ++f) {
    shEquirect.setMat4("uView", cv.views[f]);
    env_.attachFace(f, 0);
    if (!checkFbo("env capture")) return false;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawUnitCube();
  }
  // Build mip chain on env cube — the prefilter pass uses higher mip levels
  // for low-roughness samples to control variance.
  env_.bind();
  glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

  // ----- 2. Diffuse irradiance convolution --------------------------------
  const int kIrrFace = 32;
  if (!irradiance_.create(kIrrFace, GL_RGB16F, GL_RGB, GL_FLOAT, 1, true)) return false;

  sizeDepth(kIrrFace);
  shIrradiance.bind();
  shIrradiance.setInt("uEnv", 0);
  glActiveTexture(GL_TEXTURE0);
  env_.bind();
  shIrradiance.setMat4("uProj", cv.proj);
  glBindFramebuffer(GL_FRAMEBUFFER, captureFbo_);
  glViewport(0, 0, kIrrFace, kIrrFace);
  for (int f = 0; f < 6; ++f) {
    shIrradiance.setMat4("uView", cv.views[f]);
    irradiance_.attachFace(f, 0);
    if (!checkFbo("irradiance")) return false;
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawUnitCube();
  }

  // ----- 3. Prefiltered specular cubemap ----------------------------------
  const int kPrefFace = 128;
  if (!prefilter_.create(kPrefFace, GL_RGB16F, GL_RGB, GL_FLOAT, prefilterMips_, true)) return false;
  prefilter_.bind();
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  shPrefilter.bind();
  shPrefilter.setInt("uEnv", 0);
  shPrefilter.setFloat("uEnvFaceSize", static_cast<float>(kEnvFace));
  shPrefilter.setMat4("uProj", cv.proj);
  glActiveTexture(GL_TEXTURE0);
  env_.bind();
  for (int mip = 0; mip < prefilterMips_; ++mip) {
    int mipSize = kPrefFace >> mip;
    if (mipSize < 1) mipSize = 1;
    sizeDepth(mipSize);
    glBindFramebuffer(GL_FRAMEBUFFER, captureFbo_);
    glViewport(0, 0, mipSize, mipSize);
    float roughness = float(mip) / float(prefilterMips_ - 1);
    shPrefilter.setFloat("uRoughness", roughness);
    for (int f = 0; f < 6; ++f) {
      shPrefilter.setMat4("uView", cv.views[f]);
      prefilter_.attachFace(f, mip);
      if (!checkFbo("prefilter")) return false;
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      drawUnitCube();
    }
  }

  // ----- 4. BRDF integration LUT (RG16F, fullscreen pass) -----------------
  const int kLut = 512;
  glGenTextures(1, &brdfLut_);
  glBindTexture(GL_TEXTURE_2D, brdfLut_);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, kLut, kLut, 0, GL_RG, GL_FLOAT, nullptr);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  sizeDepth(kLut);
  glBindFramebuffer(GL_FRAMEBUFFER, captureFbo_);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                         GL_TEXTURE_2D, brdfLut_, 0);
  if (!checkFbo("brdf lut")) return false;
  glViewport(0, 0, kLut, kLut);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  shBrdfLut.bind();
  GLuint dummyVao = 0;
  glGenVertexArrays(1, &dummyVao);
  glBindVertexArray(dummyVao);
  glDrawArrays(GL_TRIANGLES, 0, 3);
  glDeleteVertexArrays(1, &dummyVao);

  // ----- restore + tear down attachments ----------------------------------
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
  glEnable(GL_CULL_FACE);
  glBindVertexArray(0);

  std::fprintf(stderr,
    "[ibl] baked: env %d^2 + mips, irradiance %d^2, prefilter %d^2 x %d mips, brdf %d^2\n",
    kEnvFace, kIrrFace, kPrefFace, prefilterMips_, kLut);
  return true;
}

} // namespace tyro
