#include "doctest/doctest.h"
#include "rt/core/tuple3.h"

// Define a test class to verify CRTP functionality
template <typename T>
class TestTuple : public rt::Tuple3<TestTuple<T>, T> {
public:
    using rt::Tuple3<TestTuple<T>, T>::Tuple3;
};

TEST_CASE("Tuple3 constructors and accessors") {
    TestTuple<float> t1;
    CHECK(t1.x == 0.0f);
    CHECK(t1.y == 0.0f);
    CHECK(t1.z == 0.0f);

    TestTuple<float> t2(1.0f, 2.0f, 3.0f);
    CHECK(t2.x == 1.0f);
    CHECK(t2.y == 2.0f);
    CHECK(t2.z == 3.0f);

    CHECK(t2[0] == 1.0f);
    CHECK(t2[1] == 2.0f);
    CHECK(t2[2] == 3.0f);

    t2[0] = 10.0f;
    CHECK(t2.x == 10.0f);
}

TEST_CASE("Tuple3 unary and binary operators") {
    TestTuple<float> t1(1.0f, 2.0f, 3.0f);
    TestTuple<float> t2(4.0f, 5.0f, 6.0f);

    // Unary minus
    TestTuple<float> tNeg = -t1;
    CHECK(tNeg.x == -1.0f);
    CHECK(tNeg.y == -2.0f);
    CHECK(tNeg.z == -3.0f);

    // Addition
    TestTuple<float> tAdd = t1 + t2;
    CHECK(tAdd.x == 5.0f);
    CHECK(tAdd.y == 7.0f);
    CHECK(tAdd.z == 9.0f);

    // Subtraction
    TestTuple<float> tSub = t1 - t2;
    CHECK(tSub.x == -3.0f);
    CHECK(tSub.y == -3.0f);
    CHECK(tSub.z == -3.0f);

    // Scalar multiplication
    TestTuple<float> tMul = t1 * 2.0f;
    CHECK(tMul.x == 2.0f);
    CHECK(tMul.y == 4.0f);
    CHECK(tMul.z == 6.0f);

    // Left scalar multiplication
    TestTuple<float> tMulLeft = 2.0f * t1;
    CHECK(tMulLeft.x == 2.0f);
    CHECK(tMulLeft.y == 4.0f);
    CHECK(tMulLeft.z == 6.0f);

    // Division
    TestTuple<float> tDiv = t2 / 2.0f;
    CHECK(tDiv.x == 2.0f);
    CHECK(tDiv.y == 2.5f);
    CHECK(tDiv.z == 3.0f);
}

TEST_CASE("Tuple3 compound assignments and comparison") {
    TestTuple<float> t1(1.0f, 2.0f, 3.0f);
    TestTuple<float> t2(4.0f, 5.0f, 6.0f);

    t1 += t2;
    CHECK(t1 == TestTuple<float>(5.0f, 7.0f, 9.0f));

    t1 -= t2;
    CHECK(t1 == TestTuple<float>(1.0f, 2.0f, 3.0f));

    t1 *= 2.0f;
    CHECK(t1 == TestTuple<float>(2.0f, 4.0f, 6.0f));

    t1 /= 2.0f;
    CHECK(t1 == TestTuple<float>(1.0f, 2.0f, 3.0f));

    CHECK(t1 != t2);
}

TEST_CASE("Tuple3 NaN check") {
    TestTuple<float> t1(1.0f, NAN, 3.0f);
    CHECK(t1.has_nan() == true);

    TestTuple<float> t2(1.0f, 2.0f, 3.0f);
    CHECK(t2.has_nan() == false);
}
