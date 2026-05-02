#include "ObjLoader.h"

#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_map>

namespace tyro {

namespace {
struct FaceVtx {
  int v = -1, t = -1, n = -1;
  bool operator==(const FaceVtx& o) const {
    return v == o.v && t == o.t && n == o.n;
  }
};
struct FaceVtxHash {
  size_t operator()(const FaceVtx& f) const noexcept {
    size_t h = static_cast<size_t>(f.v) * 73856093u;
    h ^= static_cast<size_t>(f.t) * 19349663u;
    h ^= static_cast<size_t>(f.n) * 83492791u;
    return h;
  }
};

// Parse "v/t/n" / "v//n" / "v/t" / "v" — values are 1-based in OBJ; we keep
// them 1-based here and resolve to 0-based on lookup.
FaceVtx parseFaceVertex(const std::string& tok) {
  FaceVtx out;
  int idx = 0;
  size_t pos = 0;
  while (pos <= tok.size() && idx < 3) {
    size_t slash = tok.find('/', pos);
    std::string sub = tok.substr(pos, (slash == std::string::npos ? tok.size() : slash) - pos);
    if (!sub.empty()) {
      int val = std::atoi(sub.c_str());
      if (idx == 0) out.v = val;
      else if (idx == 1) out.t = val;
      else out.n = val;
    }
    if (slash == std::string::npos) break;
    pos = slash + 1;
    ++idx;
  }
  return out;
}
} // namespace

bool loadObj(const char* path,
             std::vector<Vertex>& outVerts,
             std::vector<uint32_t>& outIndices) {
  std::ifstream f(path);
  if (!f) { std::fprintf(stderr, "[obj] cannot open %s\n", path); return false; }

  std::vector<Vec3> positions;
  std::vector<Vec3> normals;
  std::vector<Vec2> texcoords;
  std::vector<std::vector<FaceVtx>> faces;

  std::string line;
  while (std::getline(f, line)) {
    if (line.empty() || line[0] == '#') continue;
    std::istringstream ss(line);
    std::string head; ss >> head;
    if (head == "v") {
      Vec3 p; ss >> p.x >> p.y >> p.z; positions.push_back(p);
    } else if (head == "vn") {
      Vec3 n; ss >> n.x >> n.y >> n.z; normals.push_back(n);
    } else if (head == "vt") {
      Vec2 t; ss >> t.x >> t.y; texcoords.push_back(t);
    } else if (head == "f") {
      std::vector<FaceVtx> face;
      std::string tok;
      while (ss >> tok) face.push_back(parseFaceVertex(tok));
      if (face.size() >= 3) faces.push_back(std::move(face));
    }
  }

  // If the OBJ has no normals, generate them by averaging adjacent face normals.
  std::vector<Vec3> genNormals;
  if (normals.empty()) {
    genNormals.assign(positions.size(), Vec3{0,0,0});
    for (auto& face : faces) {
      // Triangulate fan: (0, i, i+1) for i = 1..n-2.
      Vec3 p0 = positions[face[0].v - 1];
      for (size_t i = 1; i + 1 < face.size(); ++i) {
        Vec3 p1 = positions[face[i].v - 1];
        Vec3 p2 = positions[face[i + 1].v - 1];
        Vec3 n  = normalize(cross(p1 - p0, p2 - p0));
        genNormals[face[0].v - 1]     += n;
        genNormals[face[i].v - 1]     += n;
        genNormals[face[i + 1].v - 1] += n;
      }
    }
    for (auto& n : genNormals) n = normalize(n);
  }

  // Deduplicate (v,t,n) triples into a single index buffer.
  outVerts.clear();
  outIndices.clear();
  std::unordered_map<FaceVtx, uint32_t, FaceVtxHash> seen;

  auto emit = [&](FaceVtx fv) -> uint32_t {
    auto it = seen.find(fv);
    if (it != seen.end()) return it->second;
    Vertex vx{};
    vx.position = positions[static_cast<size_t>(fv.v - 1)];
    if (!normals.empty() && fv.n > 0)
      vx.normal = normals[static_cast<size_t>(fv.n - 1)];
    else
      vx.normal = genNormals[static_cast<size_t>(fv.v - 1)];
    if (!texcoords.empty() && fv.t > 0)
      vx.uv = texcoords[static_cast<size_t>(fv.t - 1)];
    auto id = static_cast<uint32_t>(outVerts.size());
    outVerts.push_back(vx);
    seen.emplace(fv, id);
    return id;
  };

  for (auto& face : faces) {
    for (size_t i = 1; i + 1 < face.size(); ++i) {
      outIndices.push_back(emit(face[0]));
      outIndices.push_back(emit(face[i]));
      outIndices.push_back(emit(face[i + 1]));
    }
  }
  return true;
}

} // namespace tyro
