# ALBEDO: `tuple2.h` Reference

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

## 1. `Tuple2<Derived, T>`

```cpp
template <typename Derived, typename T>
class Tuple2 {
    static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");
public:
    T x, y;
```

This uses the Curiously Recurring Template Pattern: `Tuple2` is templated on the type inheriting from it, effectively letting `Point2f : Tuple2<Point2f, float>` have every operator here return a `Point2f` instead of a `Tuple2`, resolved entirely at compile time with no virtual dispatch.

The `static_assert` blocks instantiating a tulpe with a non-arithmeic `T` at compile time.

`x` and `y` are plain public members, no encapsulation. Reasonable for a primitive used constantly in hot loops (intersection tests, pixel samples) where accessor overhead isn't worth it.

### 1.1 Constructors

```cpp
constexpr Tuple2() : x(0), y(0) {}
constexpr Tuple2(T x, T y) : x(x), y(y) {
    assert(!HasNaN());
}
```

Default constructor zero-inits, no check needed since zero can't be NaN. The two-argument constructor asserts no NaN snuck in, catching a common renderer bug (a stray NaN from a bad division propagating silently downstream) right at its source. This only runs in debug builds; `assert` disappears under `NDEBUG`.

Both are `constexpr`, so a `Tuple2` can be built at compile time when its inputs are constants.
