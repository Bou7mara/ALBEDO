#include "doctest/doctest.h"
#include "rt/core/vector3.h"
#include "rt/core/point3.h"
#include "rt/core/normal3.h"
#include "rt/core/vector2.h"
#include <type_traits>

// Compile-time Type-Safety checks using SFINAE concepts
template <typename T, typename U, typename = void>
struct can_add : std::false_type {};

template <typename T, typename U>
struct can_add<T, U, std::void_t<decltype(std::declval<T>() + std::declval<U>())>> : std::true_type {};

static_assert(!can_add<rt::Point3f, rt::Point3f>::value, "Point3 + Point3 should be disabled!");
static_assert(can_add<rt::Point3f, rt::Vector3f>::value, "Point3 + Vector3 should be allowed!");
static_assert(can_add<rt::Vector3f, rt::Point3f>::value, "Vector3 + Point3 should be allowed!");

static_assert(!std::is_convertible_v<rt::Vector3f, rt::Normal3f>, "Vector3 to Normal3 conversion should be explicit!");
static_assert(std::is_constructible_v<rt::Normal3f, rt::Vector3f>, "Normal3 should be constructible from Vector3!");

TEST_CASE("Vector3 operations") {
    rt::Vector3f v1(1.0f, 2.0f, 3.0f);
    rt::Vector3f v2(4.0f, 5.0f, 6.0f);

    CHECK(rt::dot(v1, v2) == 32.0f);
    
    rt::Vector3f cross = rt::cross(v1, v2);
    CHECK(cross.x == -3.0f);
    CHECK(cross.y == 6.0f);
    CHECK(cross.z == -3.0f);

    rt::Vector3f v3(-1.0f, -2.0f, -3.0f);
    CHECK(rt::abs(v3) == v1);

    CHECK(rt::minComponent(v1) == 1.0f);
    CHECK(rt::maxComponent(v1) == 3.0f);
    CHECK(rt::maxDimension(v1) == 2);

    CHECK(rt::permute(v1, 2, 0, 1) == rt::Vector3f(3.0f, 1.0f, 2.0f));
}

TEST_CASE("Point3 and Vector3 interactions") {
    rt::Point3f p1(1.0f, 2.0f, 3.0f);
    rt::Vector3f v(10.0f, 20.0f, 30.0f);

    rt::Point3f p2 = p1 + v;
    CHECK(p2.x == 11.0f);
    CHECK(p2.y == 22.0f);
    CHECK(p2.z == 33.0f);

    rt::Vector3f diff = p2 - p1;
    CHECK(diff == v);

    rt::Point3f p3 = p2 - v;
    CHECK(p3 == p1);

    rt::Point3f pSub1(1.0f, 1.0f, 1.0f);
    rt::Point3f pSub2(4.0f, 5.0f, 1.0f);
    CHECK(rt::distanceSquared(pSub1, pSub2) == 25.0f);
    CHECK(rt::distance(pSub1, pSub2) == 5.0f);

    rt::Point3f pMin = rt::min(pSub1, pSub2);
    rt::Point3f pMax = rt::max(pSub1, pSub2);
    CHECK(pMin == rt::Point3f(1.0f, 1.0f, 1.0f));
    CHECK(pMax == rt::Point3f(4.0f, 5.0f, 1.0f));
}

TEST_CASE("Normal3 operations") {
    rt::Vector3f v(1.0f, 2.0f, 3.0f);
    rt::Normal3f n(v); // Explicit construction
    CHECK(n.x == 1.0f);
    CHECK(n.y == 2.0f);
    CHECK(n.z == 3.0f);

    rt::Normal3f n2(-1.0f, -2.0f, -3.0f);
    CHECK(rt::dot(n, v) == 14.0f);
    CHECK(rt::dot(n, n2) == -14.0f);

    rt::Normal3f nForward = rt::faceForward(n, rt::Vector3f(0.0f, 0.0f, -1.0f));
    CHECK(nForward == -n);
}

TEST_CASE("Vector2 operations") {
    rt::Vector2f v1(3.0f, 4.0f);
    CHECK(v1.Length() == 5.0f);
    CHECK(v1.Normalize() == rt::Vector2f(0.6f, 0.8f));

    rt::Vector2f v2(1.0f, 2.0f);
    CHECK(rt::dot(v1, v2) == 11.0f);
}
