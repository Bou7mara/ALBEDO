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

// Verify explicit conversion from Normal3 to Vector3
static_assert(!std::is_convertible_v<rt::Normal3f, rt::Vector3f>, "Normal3 to Vector3 conversion should be explicit!");
static_assert(std::is_constructible_v<rt::Vector3f, rt::Normal3f>, "Vector3 should be explicitly constructible from Normal3!");

TEST_CASE("Vector3 operations") {
    rt::Vector3f v1(1.0f, 2.0f, 3.0f);
    rt::Vector3f v2(4.0f, 5.0f, 6.0f);

    CHECK(rt::Dot(v1, v2) == 32.0f);
    
    rt::Vector3f cross = rt::Cross(v1, v2);
    CHECK(cross.x == -3.0f);
    CHECK(cross.y == 6.0f);
    CHECK(cross.z == -3.0f);

    rt::Vector3f v3(-1.0f, -2.0f, -3.0f);
    CHECK(rt::Abs(v3) == v1);

    CHECK(rt::MinComponent(v1) == 1.0f);
    CHECK(rt::MaxComponent(v1) == 3.0f);
    CHECK(rt::MaxDimension(v1) == 2);

    CHECK(rt::Permute(v1, 2, 0, 1) == rt::Vector3f(3.0f, 1.0f, 2.0f));

    // Verify free functions Length and Normalize
    CHECK(rt::LengthSquared(v1) == 14.0f);
    CHECK(rt::Length(rt::Vector3f(3.0f, 4.0f, 0.0f)) == 5.0f);
    CHECK(rt::Normalize(rt::Vector3f(3.0f, 4.0f, 0.0f)) == rt::Vector3f(0.6f, 0.8f, 0.0f));
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
    CHECK(rt::DistanceSquared(pSub1, pSub2) == 25.0f);
    CHECK(rt::Distance(pSub1, pSub2) == 5.0f);

    rt::Point3f pMin = rt::Min(pSub1, pSub2);
    rt::Point3f pMax = rt::Max(pSub1, pSub2);
    CHECK(pMin == rt::Point3f(1.0f, 1.0f, 1.0f));
    CHECK(pMax == rt::Point3f(4.0f, 5.0f, 1.0f));

    // Verify Lerp on Point3 builds and functions correctly
    rt::Point3f pLerp = rt::Lerp(0.5f, pSub1, pSub2);
    CHECK(pLerp == rt::Point3f(2.5f, 3.0f, 1.0f));
}

TEST_CASE("Normal3 operations") {
    rt::Vector3f v(1.0f, 2.0f, 3.0f);
    rt::Normal3f n(v); // Explicit construction
    CHECK(n.x == 1.0f);
    CHECK(n.y == 2.0f);
    CHECK(n.z == 3.0f);

    // Test explicit conversion operator Normal3 -> Vector3
    rt::Vector3f vConv(n);
    CHECK(vConv == v);

    rt::Normal3f n2(-1.0f, -2.0f, -3.0f);
    CHECK(rt::Dot(n, v) == 14.0f);
    CHECK(rt::Dot(n, n2) == -14.0f);

    rt::Normal3f nForward = rt::FaceForward(n, rt::Vector3f(0.0f, 0.0f, -1.0f));
    CHECK(nForward == -n);
}

TEST_CASE("Vector2 operations and Tuple2 CRTP base") {
    rt::Vector2f v1(3.0f, 4.0f);
    CHECK(rt::Length(v1) == 5.0f);
    CHECK(rt::Normalize(v1) == rt::Vector2f(0.6f, 0.8f));

    rt::Vector2f v2(1.0f, 2.0f);
    CHECK(rt::Dot(v1, v2) == 11.0f);
}

TEST_CASE("Integer Division Bug Check") {
    // Vector3i test (verifies conditional constexpr division does not truncate intermediate weights to 0)
    rt::Vector3i v(4, 6, 8);
    rt::Vector3i vDiv = v / 2;
    CHECK(vDiv.x == 2);
    CHECK(vDiv.y == 3);
    CHECK(vDiv.z == 4);

    vDiv /= 2;
    CHECK(vDiv.x == 1);
    CHECK(vDiv.y == 1);
    CHECK(vDiv.z == 2);

    // Vector2i test
    rt::Vector2i v2(10, 20);
    rt::Vector2i v2Div = v2 / 5;
    CHECK(v2Div.x == 2);
    CHECK(v2Div.y == 4);
}
