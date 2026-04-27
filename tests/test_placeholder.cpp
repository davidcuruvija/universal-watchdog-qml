#include <catch2/catch_test_macros.hpp>
#include "../include/placeholder.h"

TEST_CASE("Build system placeholder test", "[placeholder]") {
    REQUIRE(watchdog::add(2, 3) == 5);
    REQUIRE(watchdog::add(-1, 1) == 0);
}