#pragma once
#include <vector>
#include <cstdint>

#include "renderer/Mesh.h"

namespace tyro {

// Loads a basic Wavefront .obj file: v / vn / vt / f. N-gons are
// fan-triangulated. Missing normals are computed via face averaging;
// missing UVs default to 0. Tangents are not produced here — callers run
// computeTangents() after loading so PBR normal mapping has a TBN basis.
//
// Deliberately NOT supported: materials (mtllib), groups, smoothing groups,
// free-form geometry. Enough to read the demo's two models (Newell teapot,
// Spot the cow); add complexity only when a real model needs it.
bool loadObj(const char* path,
             std::vector<Vertex>& outVerts,
             std::vector<uint32_t>& outIndices);

} // namespace tyro
