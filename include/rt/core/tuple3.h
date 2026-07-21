#pragma once
#include <iostream>
#include <cmath>
#include <cassert>
#include <type_traits>

// Ray Tracer Core Namespace
namespace rt {
    // ==========================
    // 3D TUPLE BASE CLASS (CRTP)
    // ==========================

    // Methods ahead use constexpr for compile-time evaluation when possible
    // Curiously Recurring Template Pattern (CRTP) lets base methods return Derived objects!
    template <typename Derived, typename T>
    class Tuple3 {
        // Compile-time check: T must be an arithmetic type (int, float, double, etc.)
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    public:
        // --- Data Members ---
        T x, y, z;

        // ------------
        // CONSTRUCTORS
        // ------------

        // Default constructor (zero-initialized)
        constexpr Tuple3() : x(0), y(0), z(0) {}

        // Parameterized constructor
        constexpr Tuple3(T x, T y, T z) : x(x), y(y), z(z) {
            // Division by zero or uninitialized values yield NaN — catch edge cases early!
            assert(!HasNaN());
        }

        // ------------------------------------
        // SUBSCRIPT OPERATORS (ELEMENT ACCESS)
        // ------------------------------------

        // Const component accessor: tuple[0] -> x, tuple[1] -> y, tuple[2] -> z
        constexpr T operator[](int i) const {
            assert(i >= 0 && i < 3);
            return (i == 0) ? x : ((i == 1) ? y : z);
        }

        // Non-const component accessor & mutator: tuple[2] = val
        constexpr T& operator[](int i) {
            assert(i >= 0 && i < 3);
            return (i == 0) ? x : ((i == 1) ? y : z);
        }

        // ----------------------------------------------------
        // ARITHMETIC OPERATORS (RETURNS DERIVED TYPE VIA CRTP)
        // ----------------------------------------------------

        // Unary minus (returns component-wise negation)
        constexpr Derived operator-() const {
            return Derived(-x, -y, -z);
        }

        // Component-wise addition
        constexpr Derived operator+(const Derived& other) const {
            return Derived(x + other.x, y + other.y, z + other.z);
        }

        // Component-wise subtraction
        constexpr Derived operator-(const Derived& other) const {
            return Derived(x - other.x, y - other.y, z - other.z);
        }

        // Scalar multiplication
        constexpr Derived operator*(T scalar) const {
            return Derived(x * scalar, y * scalar, z * scalar);
        }

        // Scalar division
        constexpr Derived operator/(T scalar) const {
            assert(scalar != 0);
            if constexpr (std::is_floating_point_v<T>) {
                // Division takes 2-5x more clock cycles than multiplication on x86/ARM CPUs.
                // Invert the scalar once, then multiply 3 times to make the ALU happy!
                T inv = static_cast<T>(1) / scalar;
                return Derived(x * inv, y * inv, z * inv);
            } else {
                // Integer division: no reciprocal trick here unless you like 1/2 becoming 0!
                return Derived(x / scalar, y / scalar, z / scalar);
            }
        }

        // -----------------------------------------------------
        // COMPOUND ASSIGNMENT OPERATORS (IN-PLACE MODIFICATION)
        // -----------------------------------------------------

        // These modify the object in-place and return a reference to allow method chaining.

        // Adds the components of another tuple to this one
        constexpr Derived& operator+=(const Derived& other) {
            x += other.x;
            y += other.y;
            z += other.z;
            // Cast to Derived type using CRTP magic
            return static_cast<Derived&>(*this);
        }

        // Subtracts the components of another tuple from this one
        constexpr Derived& operator-=(const Derived& other) {
            x -= other.x;
            y -= other.y;
            z -= other.z;
            return static_cast<Derived&>(*this);
        }

        // Multiplies all three components by a scalar value
        constexpr Derived& operator*=(T scalar) {
            x *= scalar;
            y *= scalar;
            z *= scalar;
            return static_cast<Derived&>(*this);
        }

        // Divides all three components by a scalar value
        constexpr Derived& operator/=(T scalar) {
            assert(scalar != 0); // Guard against division by zero

            if constexpr (std::is_floating_point_v<T>) {
                // Multiply by inverse is the new divide.
                // Who has time for 15 clock cycles anyway?
                T inv = static_cast<T>(1) / scalar;
                x *= inv;
                y *= inv;
                z *= inv;
            } else {
                // Standard integer division, truncates toward zero
                x /= scalar;
                y /= scalar;
                z /= scalar;
            }
            return static_cast<Derived&>(*this);
        }

        // --------------------------
        // EQUALITY & UTILITY METHODS
        // --------------------------

        // Component-wise equality check
        constexpr bool operator==(const Derived& other) const {
            return x == other.x && y == other.y && z == other.z;
        }

        // Component-wise inequality check
        constexpr bool operator!=(const Derived& other) const {
            return !(*this == other);
        }

        // Checks if any component is NaN (Not a Number)
        constexpr bool HasNaN() const {
            return std::isnan(x) || std::isnan(y) || std::isnan(z);
        }
    };

    // ===========================
    // GLOBAL NON-MEMBER OPERATORS
    // ===========================

    // Weak compiler cant handle class objects on the right (scalar * tuple), soo...

    // Commutative scalar multiplication: 2.0f * vec
    template <typename Derived, typename T>
    constexpr Derived operator*(T scalar, const Tuple3<Derived, T>& tuple) {
        return static_cast<const Derived&>(tuple) * scalar;
    }

    // Stream output operator: prints [x, y, z] to ostream
    template <typename Derived, typename T>
    std::ostream& operator<<(std::ostream& os, const Tuple3<Derived, T>& tuple) {
        os << "[" << tuple.x << ", " << tuple.y << ", " << tuple.z << "]";
        return os;
    }
}
