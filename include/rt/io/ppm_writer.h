#pragma once
#include <ostream>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <string>

namespace rt {
    // Writes the PPM (P3 text format) image file header
    // Header format contains magic number P3, image width, height, and maximum color value 255
    inline void WritePPMHeader(std::ostream& os, int width, int height) {
        os << "P3\n" << width << ' ' << height << "\n255\n";
    }

    // Writes an RGB pixel color to the output stream
    // Takes floating-point color components in range [0, 1], applies gamma correction (gamma 2.2),
    // clamps values between 0 and 1, and converts them to 8-bit integer bytes (0 to 255)
    inline void WritePixel(std::ostream& os, float r, float g, float b) {
        auto toByte = [](float c) {
            c = std::clamp(c, 0.0f, 1.0f);
            c = std::pow(c, 1.0f / 2.2f); // Gamma correction converting linear color to sRGB display color
            return static_cast<int>(255.999f * c);
        };
        os << toByte(r) << ' ' << toByte(g) << ' ' << toByte(b) << '\n';
    }

    // Generates a unique output file path inside the given directory (such as "images/image1.ppm")
    // Checks existing files in the directory to increment the index and prevent overwriting past rendered images
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

