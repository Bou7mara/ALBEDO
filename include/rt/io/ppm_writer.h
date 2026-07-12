#ifndef RT_IO_PPM_WRITER_H
#define RT_IO_PPM_WRITER_H

#include <ostream>
#include <algorithm>
#include <cmath>

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

} // namespace rt

#endif // RT_IO_PPM_WRITER_H
