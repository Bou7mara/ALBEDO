#pragma once

#include <vector>
#include <cmath>
#include <algorithm>
#include <utility>

#include "rt/core/vector3.h"

#if !defined(__CUDACC__) && !defined(__CUDA_ARCH__)
#ifndef __host__
#define __host__
#endif
#ifndef __device__
#define __device__
#endif
#endif

namespace rt {

    enum class WrapMode : uint32_t {
        Repeat = 0,
        Clamp = 1
    };

    template <typename T>
    struct Image2DView {
        const T* texels;
        int width;
        int height;
        WrapMode wrap;

        Image2DView() = default;

        __host__ __device__ constexpr Image2DView(const T* data, int w, int h, WrapMode wMode = WrapMode::Repeat)
            : texels(data), width(w), height(h), wrap(wMode) {}

        __host__ __device__ T TexelAt(int ix, int iy) const {
            if (width <= 0 || height <= 0 || !texels) return T{};
            if (wrap == WrapMode::Clamp) {
                ix = (ix < 0) ? 0 : (ix >= width ? width - 1 : ix);
                iy = (iy < 0) ? 0 : (iy >= height ? height - 1 : iy);
            } else { // Repeat
                ix = (ix % width + width) % width;
                iy = (iy % height + height) % height;
            }
            return texels[iy * width + ix];
        }

        __host__ __device__ T Sample(float u, float v) const {
            if (width <= 0 || height <= 0 || !texels) return T{};

            // Convert continuous normalized UV [0, 1] to continuous pixel space [-0.5, W - 0.5]
            float x = u * static_cast<float>(width) - 0.5f;
            float y = v * static_cast<float>(height) - 0.5f;

            int x0 = static_cast<int>(floorf(x));
            int y0 = static_cast<int>(floorf(y));

            float fx = x - static_cast<float>(x0);
            float fy = y - static_cast<float>(y0);

            T t00 = TexelAt(x0,     y0);
            T t10 = TexelAt(x0 + 1, y0);
            T t01 = TexelAt(x0,     y0 + 1);
            T t11 = TexelAt(x0 + 1, y0 + 1);

            T top = (1.0f - fx) * t00 + fx * t10;
            T bot = (1.0f - fx) * t01 + fx * t11;
            return (1.0f - fy) * top + fy * bot;
        }
    };

    template <typename T>
    class Image2D {
    public:
        std::vector<T> texels;
        int width = 0;
        int height = 0;
        WrapMode wrap = WrapMode::Repeat;

        Image2D() = default;

        Image2D(int w, int h, WrapMode wMode = WrapMode::Repeat, const T& defaultVal = T{})
            : width(w), height(h), wrap(wMode), texels(w * h, defaultVal) {}

        Image2D(int w, int h, std::vector<T> data, WrapMode wMode = WrapMode::Repeat)
            : width(w), height(h), wrap(wMode), texels(std::move(data)) {}

        void Set(int x, int y, const T& val) {
            if (x >= 0 && x < width && y >= 0 && y < height) {
                texels[y * width + x] = val;
            }
        }

        [[nodiscard]] const T& Get(int x, int y) const {
            return texels[y * width + x];
        }

        [[nodiscard]] Image2DView<T> View() const {
            return Image2DView<T>(texels.data(), width, height, wrap);
        }

        [[nodiscard]] T Sample(float u, float v) const {
            return View().Sample(u, v);
        }
    };

} // namespace rt
