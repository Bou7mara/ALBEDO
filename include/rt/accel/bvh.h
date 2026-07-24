#pragma once
#include "rt/core/bounds3.h"
#include "rt/shapes/shape.h"
#include <memory>
#include <vector>

// My Ray Tracer's Core Namespace
namespace rt {

// Bounding Volume Hierarchy (BVH) class.
// A BVH is a tree data structure that groups shapes into nested (hence hierarchy)bounding boxes.
// Instead of testing aagainst every shape in the scene, ALBEDO tests
// against the bounding boxes first. If a ray misses a box, it skips all shapes inside it.

class BVH {
public:
    // Methods used to split a node into left and right children during tree construction
    // Midpoint splits the shapes along the middle of the longest bounding box axis
    // SAH uses the Surface Area Heuristic to find the split position with lower ray testing cost
    enum class SplitMethod { Midpoint, SAH };

    // Builds the BVH tree from a list of shapes
    // maxPrimsInNode sets the maximum number of shapes allowed inside a leaf node
    // splitMethod chooses how to split nodes during construction
    explicit BVH(std::vector<std::shared_ptr<Shape>> shapes,
                 int maxPrimsInNode = 4,
                 SplitMethod splitMethod = SplitMethod::SAH);

    // Tests if a ray hits any shape in the tree and calculates detailed interaction info
    bool Intersect(const Ray& ray, SurfaceInteraction* isect) const;

    // Fast check to see if a ray hits any shape in the tree
    // Useful for shadow rays where we only need a yes or no answer
    bool IntersectP(const Ray& ray) const;

    // Gets the overall bounding box enclosing all shapes in the BVH
    Bounds3f WorldBound() const;

private:
    // Temporary helper struct used while building the tree
    // Stores shape index, bounding box, and center point for sorting and splitting
    struct PrimitiveInfo {
        size_t index;       // Original index of the shape in the input list
        Bounds3f bounds;    // Bounding box of the shape
        Point3f centroid;   // Center point of the shape's bounding box
    };

    // A single node in the BVH tree
    // If nPrimitives > 0, it is a leaf node holding shapes from firstPrimOffset
    // Otherwise, it is an internal node with left and right children
    struct BVHNode {
        Bounds3f bounds;                        // Bounding box enclosing all shapes in this node
        std::unique_ptr<BVHNode> left, right;   // Pointers to left and right child nodes
        int splitAxis = 0;                      // Axis (0=X, 1=Y, 2=Z) used to split children
        int firstPrimOffset = 0;                // Offset in orderedShapes_ where leaf shapes start
        int nPrimitives = 0;                    // Number of shapes in leaf (0 for internal nodes)

        // Returns true if this node is a leaf holding shapes
        bool IsLeaf() const { return nPrimitives > 0; }
    };

    // Recursively splits shapes and builds BVH nodes from start index to end index
    std::unique_ptr<BVHNode> BuildRecursive(std::vector<PrimitiveInfo>& primInfo,
                                             int start, int end);

    // Helper to create a leaf node containing shapes from start to end
    std::unique_ptr<BVHNode> MakeLeaf(std::vector<PrimitiveInfo>& primInfo,
                                       int start, int end, const Bounds3f& bounds);

    // Helper to recursively traverse a BVH node and test ray intersection
    bool IntersectNode(const BVHNode* node, const Ray& ray, const Vector3f& invDir,
                        const int dirIsNeg[3], SurfaceInteraction* isect) const;

    // --- Data Members ---
    std::vector<std::shared_ptr<Shape>> originalShapes_; // Original list of shapes passed to constructor
    std::vector<std::shared_ptr<Shape>> orderedShapes_;  // Shapes reordered for fast memory access in leaves
    std::unique_ptr<BVHNode> root_;                      // Pointer to the root node of the tree
    int maxPrimsInNode_;                                 // Maximum shapes per leaf node
    SplitMethod splitMethod_;                            // Strategy used for splitting nodes
};
}

