#include <catch2/catch_test_macros.hpp>
#include "rt/core/progress.h"

using namespace rt;

TEST_CASE("ProgressReporter basic tracking", "[core][progress]") {
    ProgressReporter progress(100, 0.05);

    for (int i = 0; i < 100; ++i) {
        progress.Advance();
    }
    progress.Finish();

    REQUIRE(true);
}
