# ALBEDO

A physically based ray tracer written in modern C++23, made as an independent
successor to the *Ray Tracing in One Weekend* trilogy rather than a direct extension
of it.

## Status

🚧 ALBEDO is currently in active development. CPU architecture / transport algorithms are optimized. But integrators and GPU pipelines are mapped out for future development.

| System / Feature | Status | Implementation Notes |
|---|---|---|
| **Core Architecture** | 🟢 Implemented | ECS model, Thread pool, Custom math, Catch2 testing |
| **Acceleration (BVH)** | 🟢 Implemented | SAH construction, Wide BVH (BVH4/BVH8), SIMD traversal |
| **Basic Materials** | 🟢 Implemented | Lambertian, Dielectrics (Glass), Beer-Lambert |
| **Microfacet BSDFs** | 🟢 Implemented | Cook-Torrance, GGX / Beckmann distributions |
| **Advanced Materials** | ⚪ Planned | Disney Principled, Layered, Participating Media |
| **Light Transport** | 🟢 Implemented | Unidirectional Path Tracing, Direct/Indirect decomposition |
| **Sampling Methods** | 🟢 Implemented | Multiple Importance Sampling (MIS), Next Event Estimation |
| **Advanced Integrators**| ⚪ Planned | BDPT, Metropolis (MLT), ReSTIR (DI & GI) |
| **Spectral Rendering** | ⚪ Planned | Hero Wavelength Sampling, CIE matching, Dispersion |
| **Hardware / GPU** | ⚪ Planned | Megakernel/Wavefront, RTX/DXR Hardware Traversal |

*Legend: 🟢 Core Implementation Complete | 🟡 Work in Progress | ⚪ Planned*
## Why a new repository

Developing my ray tracer ultimately diverged into a dichotomy. The original repository
began as an extension of Peter Shirley's trilogy, but as it turns out, the inherited
architecture could not comfortably accommodate the scope I had envisioned. Rather than
continuing to force that codebase beyond its intended design, this repository
establishes a new lineage rather than a mere extension. It carries forward the lessons,
concepts, and experience gained from the original project, while departing from its
structural constraints and providing a new foundation for future development.

The OG, **PhotonCast**, has been made private, as it will no longer feature clean
commits and professional code going forward — from this point onwards it serves as a
sandbox and test site for ALBEDO's ideas.

## Building

Requires CMake 3.20+ and a C++23 compiler (GCC 13+, Clang 16+, or MSVC 2022 17.6+).

```bash
git clone https://github.com/Bou7mara/ALBEDO.git
cd ALBEDO
mkdir build && cd build
cmake ..
cmake --build .
```

## Testing

Tests use [Catch2](https://github.com/catchorg/Catch2), fetched automatically via
CMake's `FetchContent`.

```bash
cd build
ctest --output-on-failure
```

## License

MIT — see [LICENSE](LICENSE).
