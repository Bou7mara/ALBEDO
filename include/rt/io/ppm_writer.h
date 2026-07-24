#pragma once
#include <ostream>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

namespace rt {
    inline void WritePPMHeader(std::ostream& os, int width, int height) {
        os << "P3\n" << width << ' ' << height << "\n255\n";
    }

    inline void WritePixel(std::ostream& os, float r, float g, float b) {
        auto toByte = [](float c) {
            c = std::clamp(c, 0.0f, 1.0f);
            c = std::pow(c, 1.0f / 2.2f);
            return static_cast<int>(255.999f * c);
        };
        os << toByte(r) << ' ' << toByte(g) << ' ' << toByte(b) << '\n';
    }

    inline std::string NextImagePath(const std::string& directory = "images") {
        std::filesystem::create_directories(directory);

        int n = 1;
        std::string path;
        do {
            path = directory + "/image" + std::to_string(n) + ".ppm";
            ++n;
        } while (std::filesystem::exists(path));

        return path;
    }
}
