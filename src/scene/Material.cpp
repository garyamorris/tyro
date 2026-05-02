#include "Material.h"
#include "renderer/Shader.h"
#include "renderer/Texture.h"
#include "gl_loader.h"

namespace tyro {

void Material::apply() const {
  if (!shader) return;
  shader->bind();
  shader->setVec3 ("uAlbedo",    albedo);
  shader->setVec3 ("uEmissive",  emissive);
  shader->setFloat("uShininess", shininess);
  shader->setFloat("uMetallic",  metallic);
  shader->setFloat("uRoughness", roughness);
  shader->setVec2 ("uUvScale",   uvScale);
  shader->setVec2 ("uUvOffset",  uvOffset);

  // Albedo on TEX0
  if (albedoTex && albedoTex->valid()) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoTex->handle());
    shader->setInt  ("uAlbedoTex", 0);
    shader->setFloat("uHasAlbedo", 1.0f);
  } else {
    shader->setFloat("uHasAlbedo", 0.0f);
  }
  // Normal on TEX1
  if (normalTex && normalTex->valid()) {
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTex->handle());
    shader->setInt  ("uNormalTex", 1);
    shader->setFloat("uHasNormal", 1.0f);
  } else {
    shader->setFloat("uHasNormal", 0.0f);
  }
  // Metallic-roughness on TEX2
  if (mrTex && mrTex->valid()) {
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, mrTex->handle());
    shader->setInt  ("uMRTex", 2);
    shader->setFloat("uHasMR", 1.0f);
  } else {
    shader->setFloat("uHasMR", 0.0f);
  }
  // Restore TEX0 active for callers that don't manage units.
  glActiveTexture(GL_TEXTURE0);
}

} // namespace tyro
