#pragma once

#include "doctest.h"

// Minimal case that only proves the doctest -> CTest wiring works.
TEST_CASE("test harness reports passing assertions")
{
    CHECK(1 + 1 == 2);
}
