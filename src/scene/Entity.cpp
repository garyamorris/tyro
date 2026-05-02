#include "Entity.h"

namespace tyro {

Mat4 Entity::modelMatrix() const {
  return translate(position) * toMat4(rotation) * scale(scaling);
}

AABB Entity::worldAABB() const {
  return localAABB.transformed(modelMatrix());
}

} // namespace tyro
