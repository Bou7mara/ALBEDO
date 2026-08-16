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

|             Constant             |                                              Meaning                                               |
| :------------------------------: | :------------------------------------------------------------------------------------------------: |
|         `kRRStartDepth`          | Bounce index at which RR starts being *considered*. Depths `0,1,2` are always fully computed for.  |
| `kRRProbabilityMinimumThreshold` |    Floor on survival probability $q$: badly-attenuated paths survive with at least 50% chance.     |
| `kRRProbabilityMaximumThreshold` | Ceiling on $q$: bright paths are capped at 95% to prevent silent RR pass-through on firefly paths. |

### `0.3` `MaxChannel`

```cpp
[[nodiscard]] constexpr float MaxChannel(const Vector3f& v) {
	return std::max(v.x, std::max(v.y, v.z));
	}
```

$$
\text{MaxChannel}(\mathbf{v}) = \max(v_x, v_y, v_z)
$$
I use this to turn the path's RGB throughput into a single number for the Russian Roulette coin-flip in §1.3. I chose to use the *maximum* channel rather than something like "perceptual luminance" (or an average of the three) cause it means a path that's built up a lot of energy in even one channel (say, deep red after bouncing off a red wall several times) still gets treated as "high throughput" and is therefore kept alive, rather than being unfairly killed off because it looks dim on average. That matters for avoiding subtle color shifts that a luminance-weighted heuristic could otherwise introduce.

### `0.4` Power Heuristic (for MIS)

```cpp
inline float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
	float f = nf * fPdf;
	float g = ng * gPdf;
	return (f * f) / (f * f + g * g)
	}
```

The general form:
$$
w_f(x) = \frac{(n_f \cdot p_f(x))^2}{(n_f \cdot p_f(x))^2 + (n_g \cdot p_g(x))^2}
$$

This is Veach's power heuristic (with the exponent $2$), and it lets the integrator combine two different ways of sampling light (sampling the lights directly, and sampling the BSDF and hoping it happens to hit a light) into one low-variance estimate.

Here, `nf` and `ng` are (as a result of the implementation) always `1` (one BSDF sample, one light sample per bounce), so in practice it simplifies to just comparing the two PDFs squared: 

$$
w_f(x) = \frac{p_f(x)^2}{p_f(x)^2 + p_g(x)^2}
$$
You'll see this function called from two places, playing opposite roles each time:

- In **§1.1**, when a path stumbles onto an emitter by BSDF sampling, it weights that  contribution.
- In **§1.2**, when next-event estimation samples a light directly, it weights that contribution instead.

Note that `PowerHeustic` isn't marked `[[nodiscard]]`, `MaxChannel` on the other hand is. Weird... cause throwing away either one would be equally catastrophic. I should probably change that.
Anyways. between them, they form the two halves of an MIS estimator.
