// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

#include <kf/String.hpp>
#include <runner.hpp>

void test() {

    char buffer[100];

    kf::String my_string{{buffer}};

    my_string.append_format("Hello {}", "World");
}

void run_tests() {
    RUN_TEST(test);
}
