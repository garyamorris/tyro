#include "Scene.h"
#include "gl_loader.h"

#include <algorithm>
#include <cstdio>
#include <string>

namespace tyro {

void Scene::clearActiveScene() {
  // Materials/shaders/meshes persist across scene swaps; they're reused.
  entities.clear();
  lights.clear();
  uploadedShaders_.clear();
  octree_.clear();
}

void Scene::rebuildOctree() {
  std::vector<AABB> aabbs;
  aabbs.reserve(entities.size());

  AABB world = AABB::empty();
  for (const auto& e : entities) {
    AABB wa = e.worldAABB();
    aabbs.push_back(wa);
    world.expand(wa);
  }
  // Pad slightly so loose boundaries don't reject edge-aligned items.
  Vec3 pad{0.5f, 0.5f, 0.5f};
  world.min = world.min - pad;
  world.max = world.max + pad;

  octree_.build(aabbs, world, /*maxDepth=*/5, /*maxItemsPerNode=*/4);
}

void Scene::octreeNodeBounds(std::vector<AABB>& out) const {
  octree_.collectNodeBounds(out);
}

void Scene::cullVisible(std::vector<int>& out) const {
  Frustum f = camera.frustum();
  octree_.cull(f, out);

  // Octree culls by node bounds; do a per-entity AABB check too for tighter
  // results. Cheap given the visible set is already small.
  out.erase(std::remove_if(out.begin(), out.end(),
              [&](int i){ return !f.intersects(entities[i].worldAABB()); }),
            out.end());
}

void Scene::uploadSceneUniforms(Shader* sh) {
  if (!sh) return;
  unsigned int prog = sh->program();
  for (auto p : uploadedShaders_) if (p == prog) return; // already done this frame
  uploadedShaders_.push_back(prog);

  sh->bind();
  Mat4 view = camera.view();
  Mat4 proj = camera.projection();
  sh->setMat4("uView",      view);
  sh->setMat4("uProj",      proj);
  sh->setMat4("uViewProj",  proj * view);
  sh->setVec3("uCameraPos", camera.position);

  // Push up to 8 lights as a struct array (matches phong_lit.frag).
  int n = static_cast<int>(lights.size());
  if (n > 8) n = 8;
  sh->setInt("uLightCount", n);
  for (int i = 0; i < n; ++i) {
    const Light& L = lights[i];
    char nameBuf[64];
    auto u = [&](const char* member) {
      std::snprintf(nameBuf, sizeof(nameBuf), "uLights[%d].%s", i, member);
      return nameBuf;
    };
    sh->setInt  (u("type"),      static_cast<int>(L.type));
    sh->setVec3 (u("position"),  L.position);
    sh->setVec3 (u("direction"), normalize(L.direction));
    sh->setVec3 (u("color"),     L.color);
    sh->setFloat(u("intensity"), L.intensity);
    sh->setFloat(u("radius"),    L.radius);
  }

  // Shadow uniforms — only bind the texture if a shadow map is provided.
  // Lit shaders that don't sample uShadowMap silently ignore these.
  sh->setMat4 ("uLightVP",       lightVP);
  sh->setFloat("uShadowEnabled", (shadowEnabled && shadowMapTex != 0) ? 1.0f : 0.0f);
  if (shadowEnabled && shadowMapTex != 0) {
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, shadowMapTex);
    sh->setInt("uShadowMap", 3);
    glActiveTexture(GL_TEXTURE0);
  }

  // IBL bindings — TEX5/6/7. Shaders that don't declare these uniforms get
  // a silent no-op upload (location -1).
  bool haveIbl = iblEnabled
              && irradianceCubemap != 0
              && prefilterCubemap  != 0
              && brdfLut           != 0;
  sh->setFloat("uIblEnabled",    haveIbl ? 1.0f : 0.0f);
  sh->setFloat("uPrefilterMaxLod", static_cast<float>(prefilterMipLevels - 1));
  if (haveIbl) {
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceCubemap);
    sh->setInt("uIrradianceMap", 5);
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterCubemap);
    sh->setInt("uPrefilterMap", 6);
    glActiveTexture(GL_TEXTURE7);
    glBindTexture(GL_TEXTURE_2D, brdfLut);
    sh->setInt("uBrdfLut", 7);
    glActiveTexture(GL_TEXTURE0);
  }

  // Time, picked up by water + explode + any other animated shader.
  sh->setFloat("uTime", time);
}

void Scene::render(const std::vector<int>& visible, Shader* overrideShader) {
  uploadedShaders_.clear();

  if (overrideShader) {
    uploadSceneUniforms(overrideShader);
  }

  for (int idx : visible) {
    const Entity& e = entities[idx];
    if (!e.mesh) continue;

    Shader* sh = overrideShader;
    if (!sh && e.material) sh = e.material->shader;
    if (!sh) continue;

    if (!overrideShader) {
      uploadSceneUniforms(sh);
      if (e.material) e.material->apply();
    }

    Mat4 model = e.modelMatrix();
    Mat3 normalMat = inverseTranspose(Mat3::fromMat4Upper(model));
    sh->bind();
    sh->setMat4("uModel", model);
    sh->setMat3("uNormalMatrix", normalMat);

    if (e.material && e.material->doubleSided) glDisable(GL_CULL_FACE);
    e.mesh->draw();
    if (e.material && e.material->doubleSided) glEnable(GL_CULL_FACE);
  }
}

void Scene::setAllEntitiesMaterial(Material* m) {
  if (!m) return;
  for (auto& e : entities) e.material = m;
}

} // namespace tyro
