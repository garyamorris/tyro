#pragma once
#include <cstdint>
#include <vector>

#include "renderer/Mesh.h"

namespace tyro {

// All primitives are centered at origin and built with smooth normals where
// applicable. UVs are simple (sphere/torus: standard, plane: tiled, cylinder:
// cap UVs are radial).
void makeCube    (std::vector<Vertex>&, std::vector<uint32_t>&);
void makeSphere  (std::vector<Vertex>&, std::vector<uint32_t>&,
                  int latSegs = 16, int lonSegs = 24, float radius = 0.5f);
void makePlane   (std::vector<Vertex>&, std::vector<uint32_t>&,
                  float size = 1.0f, int subdivisions = 1);
void makeTorus   (std::vector<Vertex>&, std::vector<uint32_t>&,
                  float majorR = 0.4f, float minorR = 0.15f,
                  int majorSegs = 32, int minorSegs = 16);
void makeCylinder(std::vector<Vertex>&, std::vector<uint32_t>&,
                  float radius = 0.4f, float height = 1.0f, int segs = 24);

} // namespace tyro
