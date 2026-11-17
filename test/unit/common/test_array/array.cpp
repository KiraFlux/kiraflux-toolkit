// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

#include <kf/Array.hpp>
#include <runner.hpp>

void test() {
    kf::Array<int, 4> array{.items = {1, 2, 3, 4}};

    TEST_ASSERT_TRUE(array[0] == 1);
    TEST_ASSERT_TRUE(array[1] == 2);
    TEST_ASSERT_TRUE(array[2] == 3);
    TEST_ASSERT_TRUE(array[3] == 4);
}

void run_tests() {
    RUN_TEST(test);
}
