#pragma once
#include <iostream>
#include <cmath>
#include <cassert>
#include <type_traits>

// Ray Tracer Core Namespace
namespace rt {
    // ==========================
    // 2D TUPLE BASE CLASS (CRTP)
    // ==========================

    // Methods ahead use constexpr for compile-time evaluation when possible
    // otherwise theyre basically inline functions
    template <typename Derived, typename T>
    class Tuple2 {
        // Compile-time check: T must be an arithmetic type (int, float, double, etc.)
        static_assert(std::is_arithmetic_v<T>, "T must be an arithmetic type");

    public:
        // --- Data Members ---
        T x, y;

        // ------------
        // CONSTRUCTORS
        // ------------

        // Default constructor (zero-initialized)
        constexpr Tuple2() : x(0), y(0) {}

        // Parameterized constructor
        constexpr Tuple2(T x, T y) : x(x), y(y) {
            // 0/0 is arithmetic but yields NaN — catch edge cases early!
            assert(!HasNaN());
        }

        // ------------------------------------
        // SUBSCRIPT OPERATORS (ELEMENT ACCESS)
        // ------------------------------------

        // Const component accessor: tuple[0] -> x, tuple[1] -> y
        constexpr T operator[](int i) const {
            assert(i >= 0 && i < 2);
            return (i == 0) ? x : y;
        }

        // Non-const component accessor & mutator: tuple[0] = val
        constexpr T& operator[](int i) {
            assert(i >= 0 && i < 2);
            return (i == 0) ? x : y;
        }

        // ----------------------------------------------------
        // ARITHMETIC OPERATORS (RETURNS DERIVED TYPE VIA CRTP)
        // ----------------------------------------------------

        // Unary minus (returns negation)
        constexpr Derived operator-() const {
            return Derived(-x, -y);
        }

        // Component-wise addition
        constexpr Derived operator+(const Derived& other) const {
            return Derived(x + other.x, y + other.y);
        }

        // Component-wise subtraction
        constexpr Derived operator-(const Derived& other) const {
            return Derived(x - other.x, y - other.y);
        }

        // Scalar multiplication
        constexpr Derived operator*(T scalar) const {
            return Derived(x * scalar, y * scalar);
        }

        // Scalar division
        constexpr Derived operator/(T scalar) const {
            assert(scalar != 0);
            if constexpr (std::is_floating_point_v<T>) {
                // Division uses 2-5x more clock cycles than multiplication.
                // We divide once and multiply twice to save time!
                T inv = static_cast<T>(1) / scalar;
                return Derived(x * inv, y * inv);
            } else {
                // Scary stuff happens to integers: 1 / int = 0 -> Derived * 0 = 0
                return Derived(x / scalar, y / scalar);
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
            // cast to Derived type using CRTP magic
            return static_cast<Derived&>(*this);
        }

        // Subtracts the components of another tuple from this one
        constexpr Derived& operator-=(const Derived& other) {
            x -= other.x;
            y -= other.y;
            return static_cast<Derived&>(*this);
        }

        // Multiplies both components by a scalar value
        constexpr Derived& operator*=(T scalar) {
            x *= scalar;
            y *= scalar;
            return static_cast<Derived&>(*this);
        }

        // Divides both components by a scalar value
        constexpr Derived& operator/=(T scalar) {
            assert(scalar != 0); // Guard against division by zero

            if constexpr (std::is_floating_point_v<T>) {
                // Optimization: Multiply by the inverse to avoid slower division operations
                T inv = static_cast<T>(1) / scalar;
                x *= inv;
                y *= inv;
            } else {
                // Standard division for integers, truncation dangerous
                x /= scalar;
                y /= scalar;
            }
            return static_cast<Derived&>(*this);
        }

        // --------------------------
        // EQUALITY & UTILITY METHODS
        // --------------------------

        // Component-wise equality check
        constexpr bool operator==(const Derived& other) const {
            return x == other.x && y == other.y;
        }

        // Component-wise inequality check
        constexpr bool operator!=(const Derived& other) const {
            return !(*this == other);
        }

        // Checks if any component is NaN
        constexpr bool HasNaN() const {
            return std::isnan(x) || std::isnan(y);
        }
    };

    // ===========================
    // GLOBAL NON-MEMBER OPERATORS
    // ===========================

    // Weak compiler cant handle class objects on the right, soo...

    // Commutative multiplication
    template <typename Derived, typename T>
    constexpr Derived operator*(T scalar, const Tuple2<Derived, T>& tuple) {
        return static_cast<const Derived&>(tuple) * scalar;
    }

    // Stream output operator
    template <typename Derived, typename T>
    std::ostream& operator<<(std::ostream& os, const Tuple2<Derived, T>& tuple) {
        os << "[" << tuple.x << ", " << tuple.y << "]";
        return os;
    }
}