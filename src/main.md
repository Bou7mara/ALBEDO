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

