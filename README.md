# ALBEDO

<img width="2560" height="1280" alt="image25" src="https://github.com/user-attachments/assets/4df9eff4-4b14-4c0e-8f07-117bc12a6013" />

A physically based ray tracer built from the ground up in modern C++23.

## Status

*to be filled in later*

## Why a new repository?

Give infinitely many monkeys a keyboard, and eventually one of them will write a working ray tracer.

That was essentially the case with PhotonCast (ALBEDO's predecessor), which began as an extension of Shirley's *Ray Tracing in one Weekend* trilogy, until its architecture couldn't scale cleanly to accomodate the modularity, and performanec optimizations dictated by the scope of this project. So we're back to square one, with ALBEDO!

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
