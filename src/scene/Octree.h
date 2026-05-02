#pragma once
#include <array>
#include <memory>
#include <vector>

#include "math/AABB.h"
#include "math/Frustum.h"

namespace tyro {

// Classic recursive 8-way space partition. Built once over a static set of
// indexed AABBs; entries that span multiple children stay at the parent.
class Octree {
public:
  void build(const std::vector<AABB>& itemAABBs,
             const AABB& worldBounds,
             int maxDepth = 5,
             int maxItemsPerNode = 4);

  void clear();
  int  visibleCount() const;
  int  totalNodes()   const;

  // Append every item whose owning node intersects the frustum. Items are not
  // re-tested individually here — that's a tradeoff: faster culling, slightly
  // more entries handed to the renderer (which is doing its own per-draw work).
  void cull(const Frustum& f, std::vector<int>& outIndices) const;

private:
  struct Node {
    AABB bounds;
    std::array<std::unique_ptr<Node>, 8> children;
    std::vector<int> items;
    bool leaf = true;
  };

  std::unique_ptr<Node> root_;

  static AABB childBounds(const AABB& parent, int octant);
  void   insert(Node* n, int item, const AABB& itemAABB,
                int depth, int maxDepth, int maxItems,
                const std::vector<AABB>& itemAABBs);
  void   gatherVisible(const Node* n, const Frustum& f,
                       std::vector<int>& out) const;
  int    countNodes(const Node* n) const;
};

} // namespace tyro
