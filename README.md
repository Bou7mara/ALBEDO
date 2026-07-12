# ALBEDO

A physically based ray tracer written in modern C++23, made as an independent
successor to the *Ray Tracing in One Weekend* trilogy rather than a direct extension
of it.

## Status

🚧 Early development — math core, shapes, camera, scene, and Lambertian materials complete.

| Component            | Status       |
|-----------------------|--------------|
| Math core (Tuple/Vector/Point/Normal/Point2/Vector2) | ✅ Complete, tested |
| Ray                    | ✅ Complete, tested |
| Transform              | ✅ Complete, tested |
| Shape / Sphere         | ✅ Complete, tested |
| Camera                 | ✅ Complete, tested |
| Scene / Renderer       | ✅ Complete, tested |
| Materials / BSDFs      | ✅ Complete, tested (`Lambertian`) |
| Acceleration (BVH)     | ⬜ Not started |

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
