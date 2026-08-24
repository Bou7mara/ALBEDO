#pragma once
#include "rt/shapes/shape.h"
#include "rt/core/point2.h"
#include <vector>
#include <memory>

namespace rt {

    struct TriangleMesh {
        std::vector<Point3f> positions;
        std::vector<Normal3f> normals;
        std::vector<Point2f> uvs;
        std::vector<int> indices;

        [[nodiscard]] int TriangleCount() const noexcept {
            return static_cast<int>(indices.size() / 3);
        }
    };

    std::vector<std::shared_ptr<Shape>> MakeTriangleMesh(std::shared_ptr<TriangleMesh> mesh, std::shared_ptr<BSDF> bsdf = nullptr);

    class Triangle : public Shape {
    public:
        Triangle(std::shared_ptr<TriangleMesh> mesh, int triangleIndex, std::shared_ptr<BSDF> bsdf = nullptr)
            : Shape(std::move(bsdf)), mesh_(std::move(mesh)), TriangleIndex_(triangleIndex) {}

        bool Intersect(const Ray& ray, SurfaceInteraction* isect) const override;
        bool IntersectP(const Ray& ray) const override;
        Bounds3f WorldBound() const override;

        ShapeSample Sample(const Point3f& ref, const Point2f& u) const override;
        float Pdf(const Point3f& ref, const Vector3f& wi) const override;
        float Area() const override;

        const std::shared_ptr<TriangleMesh>& GetMesh() const { return mesh_; }
        int GetTriangleIndex() const { return TriangleIndex_; }

    private:
        [[nodiscard]] const Point3f& V0() const { return mesh_->positions[mesh_->indices[TriangleIndex_ * 3 + 0]]; }
        [[nodiscard]] const Point3f& V1() const { return mesh_->positions[mesh_->indices[TriangleIndex_ * 3 + 1]]; }
        [[nodiscard]] const Point3f& V2() const { return mesh_->positions[mesh_->indices[TriangleIndex_ * 3 + 2]]; }

        [[nodiscard]] bool HasShadingNormals() const noexcept { return !mesh_->normals.empty(); }
        [[nodiscard]] bool HasUVs() const noexcept { return !mesh_->uvs.empty(); }

        [[nodiscard]] Normal3f N0() const { return mesh_->normals[mesh_->indices[TriangleIndex_ * 3 + 0]]; }
        [[nodiscard]] Normal3f N1() const { return mesh_->normals[mesh_->indices[TriangleIndex_ * 3 + 1]]; }
        [[nodiscard]] Normal3f N2() const { return mesh_->normals[mesh_->indices[TriangleIndex_ * 3 + 2]]; }

        [[nodiscard]] Point2f UV0() const { return mesh_->uvs[mesh_->indices[TriangleIndex_ * 3 + 0]]; }
        [[nodiscard]] Point2f UV1() const { return mesh_->uvs[mesh_->indices[TriangleIndex_ * 3 + 1]]; }
        [[nodiscard]] Point2f UV2() const { return mesh_->uvs[mesh_->indices[TriangleIndex_ * 3 + 2]]; }

        std::shared_ptr<TriangleMesh> mesh_;
        int TriangleIndex_;

        static constexpr float kParallelEpsilon = 1e-8f;
    };
}
