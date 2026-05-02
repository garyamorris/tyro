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
  shader->setVec2 ("uUvScale",   uvScale);
  shader->setVec2 ("uUvOffset",  uvOffset);

  if (albedoTex && albedoTex->valid()) {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, albedoTex->handle());
    shader->setInt  ("uAlbedoTex", 0);
    shader->setFloat("uHasAlbedo", 1.0f);
  } else {
    shader->setFloat("uHasAlbedo", 0.0f);
  }
}

} // namespace tyro
