# ALBEDO: `main.cpp` Reference

> Documentation for the driver file: the path integrator `ALBEDO()`, the render loop, threading/tiling, and output. Assumes chapters on BSDF, Lights, Scene, and Russian Roulette have already been read.

---

## `0.` Boiler-plate

### `0.1` Includes

```cpp
#include <fstream>
#include <thread>

#include "rt/core/point2.h"
#include "rt/core/rng.h"
#include "rt/cam/perspective_camera.h"
#include "rt/shapes/sphere.h"
#include "rt/materials/lambertian.h"
#include "rt/materials/metal.h"
#include "rt/materials/emissive.h"
#include "rt/scene/scene.h"
#include "rt/showcase.h"
#include "rt/io/ppm_writer.h"
#include "rt/png_writer.h"
#include "rt/progress"
```

Notable absence: no `<atomic>` include despite `std::atomic<int>` being used further down. Code still compiles given it is included in ...

In Progress: Compilation times are annoyingly slow right now, and so I'm working on getting modules to work: planning to use them both as `import std;` and to export all header files as modules.

### `0.2` Constants

```cpp
namespace {
	constexpr int kRRStartDepth = 3;
	constexpr float kRRProbabilityMinimumThreshold = 0.5f;
	constexpr float kRRProbabilityMaximumThreshold = 0.95f;
	}
```

Anonymous namespace → internal linkage, these constants are private to this translation unit. They're the three knobs I used parameterize my Russian Roulette technique (§1.3 below):

