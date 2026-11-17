// SPDX-FileCopyrightText: 2026 KiraFlux
// SPDX-License-Identifier: GPL-3.0-or-later
// KiraFlux Toolkit - https://github.com/KiraFlux/kiraflux-toolkit

#include <kf/StringView.hpp>
#include <runner.hpp>

using kf::Option;
using kf::Slice;
using kf::StringView;

static void assert_string_view_equal(StringView actual, char const *expected, size_t len) {
    TEST_ASSERT_EQUAL_UINT32(len, actual.length());

    if (len) {
        TEST_ASSERT_EQUAL_MEMORY(expected, actual.data(), len);
    }
}

namespace constructors {

void default_case() {
    StringView sv{};

    TEST_ASSERT_NULL(sv.data());
    TEST_ASSERT_EQUAL_UINT32(0, sv.length());
    TEST_ASSERT_TRUE(sv.empty());
}

void cstring() {
    char const *s{"hello"};
    StringView sv{s};

    TEST_ASSERT_EQUAL_PTR(s, sv.data());
    TEST_ASSERT_EQUAL_UINT32(5, sv.length());
}

void ptr_size() {
    char const *d{"world"};
    StringView sv{d, 3};

    TEST_ASSERT_EQUAL_PTR(d, sv.data());
    TEST_ASSERT_EQUAL_UINT32(3, sv.length());
    TEST_ASSERT_EQUAL('w', sv[0]);
    TEST_ASSERT_EQUAL('o', sv[1]);
    TEST_ASSERT_EQUAL('r', sv[2]);
}

void literal() {
    StringView sv{"literal"};

    TEST_ASSERT_EQUAL_UINT32(7, sv.length());
    TEST_ASSERT_EQUAL_STRING("literal", sv.data());
}

void slice() {
    char const *d{"slice"};
    Slice<char const> sl{d, 5};
    StringView sv{d, sl.length()};

    TEST_ASSERT_EQUAL_PTR(d, sv.data());
    TEST_ASSERT_EQUAL_UINT32(5, sv.length());
}

void null_cstring() {
    StringView sv{nullptr};

    TEST_ASSERT_NULL(sv.data());
    TEST_ASSERT_EQUAL_UINT32(0, sv.length());
    TEST_ASSERT_TRUE(sv.empty());
}

}// namespace constructors

namespace test_access {

void iterators() {
    StringView sv{"12345"};
    char const *it{sv.begin()};

    TEST_ASSERT_EQUAL('1', *it);
    it += 1;

    TEST_ASSERT_EQUAL('2', *it);
    it += 1;

    TEST_ASSERT_EQUAL('3', *it);
    it += 1;

    TEST_ASSERT_EQUAL('4', *it);
    it += 1;

    TEST_ASSERT_EQUAL('5', *it);
    it += 1;

    TEST_ASSERT_EQUAL(sv.end(), it);
}

}// namespace test_access

namespace compare {

void equality() {
    StringView a{"hello"};
    StringView b{"hello"};
    StringView c{"world"};

    TEST_ASSERT_TRUE(a == b);
    TEST_ASSERT_FALSE(a != b);
    TEST_ASSERT_TRUE(a != c);
}

}// namespace compare

namespace find {

void character() {
    StringView sv{"hello world"};
    auto p{sv.index_of('o')};

    TEST_ASSERT_TRUE(p.is_some() and p.unwrap() == 4);

    p = sv.index_of('o');

    TEST_ASSERT_TRUE(p.is_some() and p.unwrap() == 4);

    StringView sub{sv.sub(5)};
    p = sub.index_of('o');

    TEST_ASSERT_TRUE(p.is_some() and p.unwrap() == 2);

    p = sv.index_of('x');

    TEST_ASSERT_TRUE(p.is_none());
}

void substring() {
    StringView sv{"the quick brown fox"};
    auto p{sv.index_of("quick")};
    TEST_ASSERT_TRUE(p.is_some() and p.unwrap() == 4);

    p = sv.index_of("brown");
    TEST_ASSERT_TRUE(p.is_some());
    TEST_ASSERT_EQUAL_INT32(p.unwrap(), 10);

    p = sv.index_of("cat");
    TEST_ASSERT_FALSE(p.is_some());

    p = sv.index_of("");
    TEST_ASSERT_TRUE(p.is_none());

    p = sv.index_of('?');
    TEST_ASSERT_TRUE(p.is_none());
}

void rfind() {
    StringView sv{"abracadabra"};

    auto p{sv.last_index_of('a')};
    TEST_ASSERT_TRUE(p.is_some() and p.unwrap() == 10);

    auto p2{sv.last_index_of('x')};
    TEST_ASSERT_TRUE(p2.is_none());
}

}// namespace find

namespace starts_ends {

void test() {
    StringView sv{"hello.txt"};

    TEST_ASSERT_TRUE(sv.starts_with("hello"));
    TEST_ASSERT_TRUE(sv.ends_with(".txt"));
    TEST_ASSERT_FALSE(sv.starts_with("world"));
    TEST_ASSERT_FALSE(sv.ends_with(".cpp"));
}

}// namespace starts_ends

namespace sub {

void sub() {
    StringView sv{"hello world"};
    auto s{sv.sub(6, kf::some<kf::usize>(5))};

    assert_string_view_equal(s, "world", 5);

    s = sv.sub(6, kf::some<kf::usize>(100));

    assert_string_view_equal(s, "world", 5);

    s = sv.sub(20, kf::some<kf::usize>(5));

    TEST_ASSERT_TRUE(s.empty());

    s = sv.sub(3, kf::some<kf::usize>(0));

    TEST_ASSERT_TRUE(s.empty());
}

void from_offset() {
    StringView sv{"hello world"};
    auto s{sv.from_offset(6)};

    assert_string_view_equal(s, "world", 5);

    s = sv.from_offset(20);

    TEST_ASSERT_TRUE(s.empty());

    s = sv.from_offset(0);

    TEST_ASSERT_EQUAL_UINT32(11, s.length());
}

}// namespace sub

namespace crop {

void remove_prefix() {
    StringView sv{"hello world"};
    sv = sv.remove_prefix("hello ");

    assert_string_view_equal(sv, "world", 5);

    sv = sv.remove_prefix("xyz");

    assert_string_view_equal(sv, "world", 5);
}

void remove_suffix() {
    StringView sv{"hello world"};
    sv = sv.remove_suffix(" world");

    assert_string_view_equal(sv, "hello", 5);

    sv = sv.remove_suffix("xyz");

    assert_string_view_equal(sv, "hello", 5);
}

}// namespace crop

namespace trim {

void start() {
    assert_string_view_equal(StringView{"   hello"}.trim_start(), "hello", 5);
}

void end() {
    assert_string_view_equal(StringView{"hello   "}.trim_end(), "hello", 5);
}

void both() {
    assert_string_view_equal(StringView{"   hello world   "}.trim(), "hello world", 11);
    assert_string_view_equal(StringView{""}.trim(), "", 0);
    assert_string_view_equal(StringView{"   "}.trim(), "", 0);
    assert_string_view_equal(StringView{"  a  "}.trim(), "a", 1);
}

}// namespace trim

namespace to_slice {

void test() {
    StringView sv{"test"};
    auto sl{sv.slice()};

    TEST_ASSERT_EQUAL_PTR(sv.data(), sl.data());
    TEST_ASSERT_EQUAL_UINT32(sv.length(), sl.length());
}

}// namespace to_slice

void run_tests() {
    RUN_TEST(constructors::default_case);
    RUN_TEST(constructors::cstring);
    RUN_TEST(constructors::ptr_size);
    RUN_TEST(constructors::literal);
    RUN_TEST(constructors::slice);
    RUN_TEST(constructors::null_cstring);

    RUN_TEST(test_access::iterators);

    RUN_TEST(compare::equality);

    RUN_TEST(find::character);
    RUN_TEST(find::substring);
    RUN_TEST(find::rfind);

    RUN_TEST(starts_ends::test);

    RUN_TEST(sub::sub);
    RUN_TEST(sub::from_offset);

    RUN_TEST(crop::remove_prefix);
    RUN_TEST(crop::remove_suffix);

    RUN_TEST(trim::start);
    RUN_TEST(trim::end);
    RUN_TEST(trim::both);

    RUN_TEST(to_slice::test);
}