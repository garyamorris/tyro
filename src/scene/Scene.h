#pragma once
#include <memory>
#include <vector>

#include "math/AABB.h"
#include "renderer/Mesh.h"
#include "renderer/Shader.h"
#include "scene/Camera.h"
#include "scene/Entity.h"
#include "scene/Light.h"
#include "scene/Material.h"
#include "scene/Octree.h"

namespace tyro {

// Scene owns the GPU-resident resources (meshes, shaders) and the
// CPU-side material/entity lists. Entities reference these by raw pointer.
class Scene {
public:
  Camera                                       camera;
  std::vector<Light>                           lights;
  std::vector<Entity>                          entities;
  std::vector<std::unique_ptr<Mesh>>           meshes;
  std::vector<std::unique_ptr<Shader>>         shaders;
  std::vector<std::unique_ptr<Material>>       materials;

  // Shadow data set externally each frame; uploaded to lit shaders during
  // uploadSceneUniforms so shaders can do PCF against the sun's depth map.
  Mat4         lightVP        = Mat4::identity();
  unsigned int shadowMapTex   = 0;
  bool         shadowEnabled  = false;

  // IBL inputs (set externally by the app once the bake is done). When
  // iblEnabled is true, the PBR shader replaces its constant ambient term
  // with a full split-sum IBL contribution.
  unsigned int irradianceCubemap = 0;
  unsigned int prefilterCubemap  = 0;
  unsigned int brdfLut           = 0;
  int          prefilterMipLevels = 5;
  bool         iblEnabled        = false;

  // Wall-clock time in seconds, set by the app. Shaders sample this for
  // animation (water displacement, explode pulse, scrolling UVs, etc.).
  float        time           = 0.0f;

  // Drop entities, lights, and materials. Meshes and shaders persist so
  // the next scene builder can wire them straight back in.
  void clearActiveScene();

  // Build/refresh the octree from current entity world AABBs. Call after the
  // entity list changes; for static scenes call once.
  void rebuildOctree();

  // Returns indices into `entities` that survive frustum culling.
  void cullVisible(std::vector<int>& outIndices) const;

  int  totalOctreeNodes() const { return octree_.totalNodes(); }

  // Issue draw calls for the visible entity set. Uploads per-entity model
  // transforms; per-frame camera/light uniforms are uploaded once per shader
  // first time it's bound this frame.
  void render(const std::vector<int>& visible, Shader* overrideShader = nullptr);

  // Force-set a single material on every entity (demo: hotkey-swappable shading).
  void setAllEntitiesMaterial(Material* m);

private:
  Octree octree_;

  // Per-frame: track which shaders we've already pushed scene uniforms into,
  // so we don't re-upload camera/light arrays for every draw call.
  std::vector<unsigned int> uploadedShaders_;
  void uploadSceneUniforms(Shader* sh);
};

} // namespace tyro
