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

### 1.2 Indexing

```cpp
constexpr T operator[](int i) const { ... }
constexpr T& operator[](int i) { ... }
```

Const and non-const overloads for array-style access, both bounds-checked via `assert`. In a release build, an out-of-range index silently falls through to returning `y`, since the ternary only branches on `i == 0`. Worth remembering if odd behavior shows up on bad indices in optimized builds.

### 1.3 Arithmetic

```cpp
constexpr Derived operator-() const { return Derived(-x, -y); }
constexpr Derived operator+(const Derived& other) const { return Derived(x + other.x, y + other.y); }
constexpr Derived operator-(const Derived& other) const { return Derived(x - other.x, y - other.y); }
constexpr Derived operator*(T scalar) const { return Derived(x * scalar, y * scalar); }
constexpr Derived operator/(T scalar) const {
    assert(scalar != 0);
    if constexpr (std::is_floating_point_v<T>) {
        T inv = static_cast<T>(1) / scalar;
        return Derived(x * inv, y * inv);
    } else {
        return Derived(x / scalar, y / scalar);
    }
}
```

Negation, addition, subtraction are all plain component-wise ops that return a `Derived`, this is the CRTP payoff, `a + b` on two `Point2f` gives back a `Point2f` with no casting.

Division asserts against a zero divisor even for floating-point `T`, where IEEE 754 would happily produce `inf` or `NaN`. That's stricter-than-necessary but its my policy throughout... consistent with catching catching bad values at the source instead of letting them propagate.

The `if constexpr` branch computes a reciprocal once and multiplies for floating-point `T` (cheaper than repeated division), and falls back to plain division for integers, where a reciprocal doesn't make sense. Since it's `if constexpr`, only one branch is ever compiled per instantiation, no runtime cost either way.
