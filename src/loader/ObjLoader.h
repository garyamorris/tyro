#pragma once
#include <vector>
#include <cstdint>

#include "renderer/Mesh.h"

namespace tyro {

// Loads a basic Wavefront .obj file: v / vn / vt / f. Quads are triangulated.
// Missing normals are computed via face averaging. Missing UVs default to 0.
bool loadObj(const char* path,
             std::vector<Vertex>& outVerts,
             std::vector<uint32_t>& outIndices);

} // namespace tyro
