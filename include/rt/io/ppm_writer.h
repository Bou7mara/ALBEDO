#pragma once
#include <ostream>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

namespace rt {
    // Writes a P3 (ASCII) PPM header. Call once before writing pixels.
    inline void WritePPMHeader(std::ostream& os, int width, int height) {
        os << "P3\n" << width << ' ' << height << "\n255\n";
    }

    // Writes a single pixel's color, given linear [0,1] RGB components.
    // Clamps and gamma-corrects (simple 1/2.2 power, matching RTIOW's
    // convention) before converting to 0-255 integers.
    inline void WritePixel(std::ostream& os, float r, float g, float b) {
        auto toByte = [](float c) {
            c = std::clamp(c, 0.0f, 1.0f);
            c = std::pow(c, 1.0f / 2.2f);   // gamma correction
            return static_cast<int>(255.999f * c);
        };
        os << toByte(r) << ' ' << toByte(g) << ' ' << toByte(b) << '\n';
    }

    // Returns the path for the next numbered image (images/image1.ppm,
    // images/image2.ppm, ...), creating the images/ directory if it
    // doesn't exist yet. Numbering is derived by checking what already
    // exists on disk -- no separate counter file or state to keep in sync.
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
