#pragma once
#include <vector>

#include "Cubemap.h"
#include "math/Math.h"

namespace tyro {

class Shader;

// Generate a procedural HDR equirect "sky": a smooth horizon→zenith gradient
// plus a bright sun disc and a warm ground tint. Output is float RGB at
// `width x height`. Tileable horizontally.
//
// `sunDir` points *toward* the sun (i.e. the positive light direction); pass
// the negation of your scene's directional-light direction for consistency.
std::vector<float> makeProceduralSkyHDR(int width, int height,
                                        Vec3 sunDir, float sunIntensity = 80.0f);

// Upload an equirect float HDR image to a freshly-allocated GL_TEXTURE_2D
// (RGB16F, clamp-to-edge in V, repeat in U). Returns the GL handle; the
// caller is responsible for glDeleteTextures() when no longer needed.
unsigned int uploadEquirectHDR(const float* rgbF, int width, int height);

// Loads a Radiance .hdr / OpenEXR-equivalent equirect file from disk via
// stb_image (`stbi_loadf`). Returns an empty vector on failure (file
// missing or unsupported format). On success `outW`/`outH` are set and
// the buffer is interleaved RGB float in scanline order — top-of-sphere
// at row 0, matching the convention of `makeProceduralSkyHDR`.
std::vector<float> loadEquirectHDRFromFile(const char* path,
                                           int& outW, int& outH);

// Bakes the four IBL inputs from an equirect HDR source. Owns its own
// resources; produced cubemaps + 2D LUT live as long as the IblBaker.
//
// Usage:
//   IblBaker baker;
//   baker.bakeFromEquirect(equirectFloatTex, equirectW, equirectH);
//   ... use baker.environmentMap(), .irradianceMap(), .prefilterMap(), .brdfLut() ...
class IblBaker {
public:
  IblBaker() = default;
  ~IblBaker();
  IblBaker(const IblBaker&) = delete;
  IblBaker& operator=(const IblBaker&) = delete;

  // Bake from a GL 2D texture handle that contains an equirectangular HDR
  // image (RGB16F or RGB32F). The handle is sampled but not owned.
  bool bakeFromEquirect(unsigned int equirectTex, int srcW, int srcH);

  // The full-resolution radiance cubemap (used by the skybox pass).
  const Cubemap& environmentMap() const { return env_;        }
  // Diffuse irradiance — sampled by PBR shader for the kD term.
  const Cubemap& irradianceMap()  const { return irradiance_; }
  // Prefiltered radiance map — sampled by PBR shader for the kS term.
  const Cubemap& prefilterMap()   const { return prefilter_;  }
  // 2D split-sum LUT (RG16F).
  unsigned int   brdfLut()        const { return brdfLut_;    }

  int prefilterMipLevels() const { return prefilterMips_; }

  void destroy();

private:
  Cubemap env_;
  Cubemap irradiance_;
  Cubemap prefilter_;
  unsigned int brdfLut_ = 0;
  int prefilterMips_ = 5;

  // GPU resources used during the bake (created on first call, destroyed by
  // dtor). Cube mesh used for the capture passes.
  unsigned int cubeVao_ = 0, cubeVbo_ = 0;
  unsigned int captureFbo_ = 0, captureRbo_ = 0;

  void initCaptureGeometry();
  void drawUnitCube() const;
};

} // namespace tyro
