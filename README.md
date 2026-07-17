# ALBEDO

A physically based ray tracer built from the ground up in modern C++23.

## Status

🚧 **ALBEDO is currently under development.**

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

## Why a new repository?

This project represents a clean architectural break from my previous ray tracer, **PhotonCast**. 

While PhotonCast began as an extension of Peter Shirley's excellent *Ray Tracing in One Weekend* trilogy, its inherited educational architecture ultimately could not scale to accommodate the scope, modularity, and performance optimizations required for a production-oriented renderer. 

Rather than fighting the structural constraints of the old codebase, ALBEDO was designed from scratch. It carries forward the mathematical foundation and rendering concepts gained from the original project, but establishes a highly decoupled, modern C++ foundation built for extensibility. 

Moving forward, ALBEDO will maintain a strict standard for clean architecture, professional code quality, and structured commit history. The original PhotonCast repository has been made private and now serves purely as a sandbox for rapid prototyping and testing messy ideas before they are formalized here.

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
