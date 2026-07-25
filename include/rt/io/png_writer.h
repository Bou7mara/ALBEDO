#pragma once
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "rt/io/stb_image_write.h"
#include "rt/core/vector3.h"

#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace rt {
    inline bool WritePNG(const std::string& filename, int width, int height, const std::vector<Vector3f>& buffer) {
        std::vector<uint8_t> pixels(width * height * 3);
        
        for (size_t i = 0; i < buffer.size(); ++i) {
            auto toByte = [](float c) {
                c = std::clamp(c, 0.0f, 1.0f);
                c = std::pow(c, 1.0f / 2.2f);
                return static_cast<uint8_t>(255.999f * c);
            };

            pixels[i * 3 + 0] = toByte(buffer[i].x);
            pixels[i * 3 + 1] = toByte(buffer[i].y);
            pixels[i * 3 + 2] = toByte(buffer[i].z);
        }

        return stbi_write_png(filename.c_str(), width, height, 3, pixels.data(), width * 3) != 0;
    }
}
