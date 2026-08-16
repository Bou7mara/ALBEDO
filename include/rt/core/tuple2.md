# ALBEDO — `tuple2.h` Reference

> `Tuple2<Derived, T>`, the CRTP base class behind every 2-component math type in the renderer (`Point2f`, `Vector2f`, etc).

---

## 0. Boilerplate

```cpp
#pragma once
#include <iostream>
#include <cmath>
#include <cassert>
#include <type_traits>
```

Trivial stuff: `<iostream>` for input and output, `<cmath>` for `std::isnan`, `<cassert>` for the asserts throughout, `<type_traits>` for the compile-time checks on `T`.

---
