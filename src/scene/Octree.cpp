#include "Octree.h"

namespace tyro {

AABB Octree::childBounds(const AABB& parent, int octant) {
  Vec3 mid = parent.center();
  Vec3 mn  = parent.min;
  Vec3 mx  = parent.max;
  // Bit layout: x = bit 0, y = bit 1, z = bit 2.
  AABB c;
  c.min.x = (octant & 1) ? mid.x : mn.x;
  c.max.x = (octant & 1) ? mx.x  : mid.x;
  c.min.y = (octant & 2) ? mid.y : mn.y;
  c.max.y = (octant & 2) ? mx.y  : mid.y;
  c.min.z = (octant & 4) ? mid.z : mn.z;
  c.max.z = (octant & 4) ? mx.z  : mid.z;
  return c;
}

void Octree::clear() { root_.reset(); }

void Octree::insert(Node* n, int item, const AABB& itemAABB,
                    int depth, int maxDepth, int maxItems,
                    const std::vector<AABB>& itemAABBs) {
  // Subdivide if this leaf has overflowed and has room to split.
  if (n->leaf && depth < maxDepth &&
      static_cast<int>(n->items.size()) >= maxItems) {
    n->leaf = false;
    // Re-distribute existing items.
    std::vector<int> stash;
    stash.swap(n->items);
    for (int prev : stash) {
      insert(n, prev, itemAABBs[prev], depth, maxDepth, maxItems, itemAABBs);
    }
  }

  if (n->leaf) {
    n->items.push_back(item);
    return;
  }

  // Find a child that fully contains the item; otherwise keep at this level.
  for (int o = 0; o < 8; ++o) {
    AABB cb = childBounds(n->bounds, o);
    if (cb.contains(itemAABB)) {
      if (!n->children[o]) {
        auto c = std::make_unique<Node>();
        c->bounds = cb;
        n->children[o] = std::move(c);
      }
      insert(n->children[o].get(), item, itemAABB,
             depth + 1, maxDepth, maxItems, itemAABBs);
      return;
    }
  }
  n->items.push_back(item);
}

void Octree::build(const std::vector<AABB>& itemAABBs,
                   const AABB& worldBounds,
                   int maxDepth, int maxItemsPerNode) {
  root_ = std::make_unique<Node>();
  root_->bounds = worldBounds;
  for (size_t i = 0; i < itemAABBs.size(); ++i) {
    insert(root_.get(), static_cast<int>(i), itemAABBs[i],
           0, maxDepth, maxItemsPerNode, itemAABBs);
  }
}

void Octree::gatherVisible(const Node* n, const Frustum& f,
                           std::vector<int>& out) const {
  if (!n) return;
  if (!f.intersects(n->bounds)) return;
  for (int idx : n->items) out.push_back(idx);
  if (!n->leaf) {
    for (auto& c : n->children) if (c) gatherVisible(c.get(), f, out);
  }
}

void Octree::cull(const Frustum& f, std::vector<int>& outIndices) const {
  outIndices.clear();
  gatherVisible(root_.get(), f, outIndices);
}

int Octree::countNodes(const Node* n) const {
  if (!n) return 0;
  int c = 1;
  for (auto& ch : n->children) if (ch) c += countNodes(ch.get());
  return c;
}
int Octree::totalNodes() const { return countNodes(root_.get()); }
int Octree::visibleCount() const { return 0; /* unused */ }

} // namespace tyro
