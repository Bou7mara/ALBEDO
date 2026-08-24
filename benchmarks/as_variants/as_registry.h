#pragma once
#include "rt/accel/blas.h"
#include "rt/accel/bvh.h"
#include "rt/accel/bvh4.h"
#include "rt/accel/bvh8.h"
#include "rt/accel/tlas.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace rt::bench {

struct ASDescriptor {
    std::string name;
    std::function<std::unique_ptr<BLAS>(const std::vector<std::shared_ptr<Shape>>&)> buildFn;
    std::function<size_t(const BLAS&)> memoryFn;
    std::function<size_t(const BLAS&)> nodeCountFn;
    std::function<float(const BLAS&)> fanoutFn;
};

inline std::vector<ASDescriptor> GetStandardASRegistry(bool isInstanced = false) {
    if (isInstanced) {
        return {
            {
                "TLAS (Instanced BVH)",
                [](const std::vector<std::shared_ptr<Shape>>& shapes) -> std::unique_ptr<BLAS> {
                    std::vector<std::shared_ptr<Instance>> instances;
                    instances.reserve(shapes.size());
                    for (const auto& s : shapes) {
                        if (auto inst = std::dynamic_pointer_cast<Instance>(s)) {
                            instances.push_back(inst);
                        }
                    }
                    return std::make_unique<TLAS>(instances);
                },
                [](const BLAS& as) -> size_t {
                    const auto* tlas = dynamic_cast<const TLAS*>(&as);
                    return (tlas && tlas->GetBVH()) ? tlas->GetBVH()->NodeCount() * 32 : 0;
                },
                [](const BLAS& as) -> size_t {
                    const auto* tlas = dynamic_cast<const TLAS*>(&as);
                    return (tlas && tlas->GetBVH()) ? tlas->GetBVH()->NodeCount() : 0;
                },
                [](const BLAS&) -> float { return 2.0f; }
            },
            {
                "Flat BVH4 (Over Instances)",
                [](const std::vector<std::shared_ptr<Shape>>& shapes) -> std::unique_ptr<BLAS> {
                    return std::make_unique<BVH4>(shapes, 4);
                },
                [](const BLAS& as) -> size_t {
                    const auto* bvh4 = dynamic_cast<const BVH4*>(&as);
                    return bvh4 ? bvh4->NodeCount() * sizeof(WideBVHNode<4>) : 0;
                },
                [](const BLAS& as) -> size_t {
                    const auto* bvh4 = dynamic_cast<const BVH4*>(&as);
                    return bvh4 ? bvh4->NodeCount() : 0;
                },
                [](const BLAS& as) -> float {
                    const auto* bvh4 = dynamic_cast<const BVH4*>(&as);
                    return bvh4 ? bvh4->AverageChildrenPerNode() : 0.0f;
                }
            }
        };
    }

    return {
        {
            "Binned-SAH (Serial)",
            [](const std::vector<std::shared_ptr<Shape>>& shapes) -> std::unique_ptr<BLAS> {
                return std::make_unique<BVH>(shapes, 4, BVH::SplitMethod::SAH, 1);
            },
            [](const BLAS& as) -> size_t {
                const auto* bvh = dynamic_cast<const BVH*>(&as);
                return bvh ? bvh->NodeCount() * 32 : 0;
            },
            [](const BLAS& as) -> size_t {
                const auto* bvh = dynamic_cast<const BVH*>(&as);
                return bvh ? bvh->NodeCount() : 0;
            },
            [](const BLAS&) -> float { return 2.0f; }
        },
        {
            "Binned-SAH (Parallel)",
            [](const std::vector<std::shared_ptr<Shape>>& shapes) -> std::unique_ptr<BLAS> {
                return std::make_unique<BVH>(shapes, 4, BVH::SplitMethod::SAH, 0);
            },
            [](const BLAS& as) -> size_t {
                const auto* bvh = dynamic_cast<const BVH*>(&as);
                return bvh ? bvh->NodeCount() * 32 : 0;
            },
            [](const BLAS& as) -> size_t {
                const auto* bvh = dynamic_cast<const BVH*>(&as);
                return bvh ? bvh->NodeCount() : 0;
            },
            [](const BLAS&) -> float { return 2.0f; }
        },
        {
            "Wide BVH4 (4-wide SIMD)",
            [](const std::vector<std::shared_ptr<Shape>>& shapes) -> std::unique_ptr<BLAS> {
                return std::make_unique<BVH4>(shapes, 4);
            },
            [](const BLAS& as) -> size_t {
                const auto* bvh4 = dynamic_cast<const BVH4*>(&as);
                return bvh4 ? bvh4->NodeCount() * sizeof(WideBVHNode<4>) : 0;
            },
            [](const BLAS& as) -> size_t {
                const auto* bvh4 = dynamic_cast<const BVH4*>(&as);
                return bvh4 ? bvh4->NodeCount() : 0;
            },
            [](const BLAS& as) -> float {
                const auto* bvh4 = dynamic_cast<const BVH4*>(&as);
                return bvh4 ? bvh4->AverageChildrenPerNode() : 0.0f;
            }
        },
        {
            "Wide BVH8 (8-wide AVX2)",
            [](const std::vector<std::shared_ptr<Shape>>& shapes) -> std::unique_ptr<BLAS> {
                return std::make_unique<BVH8>(shapes, 4);
            },
            [](const BLAS& as) -> size_t {
                const auto* bvh8 = dynamic_cast<const BVH8*>(&as);
                return bvh8 ? bvh8->NodeCount() * sizeof(WideBVHNode<8>) : 0;
            },
            [](const BLAS& as) -> size_t {
                const auto* bvh8 = dynamic_cast<const BVH8*>(&as);
                return bvh8 ? bvh8->NodeCount() : 0;
            },
            [](const BLAS& as) -> float {
                const auto* bvh8 = dynamic_cast<const BVH8*>(&as);
                return bvh8 ? bvh8->AverageChildrenPerNode() : 0.0f;
            }
        }
    };
}

}
