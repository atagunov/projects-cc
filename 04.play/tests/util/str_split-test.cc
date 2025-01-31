#include <util/str_split.h>
#include <gtest/gtest.h>
#include <string_view>
#include <iostream>
#include <span>
#include <algorithm>

using namespace std::string_view_literals;

template<typename EXPECTED>
std::string runTest(const util::str_split::StrSplitView<std::string_view> view, EXPECTED&& expected) {
    std::ostringstream s;

    if (!std::ranges::equal(view, expected)) {
        std::ranges::copy(view, std::ostream_iterator<std::string_view>(s, ", "));
    }

    return (std::move(s)).str();
}

TEST(str_split, test) {
    /* one way to create expected view is to use a separate array lvalue, then we can construct a span */
    util::str_split::StrSplitView view("abc\ncde"sv);
    std::string_view expectedA[] = {"abc", "cde"};
    EXPECT_TRUE(std::ranges::equal(view, std::span(expectedA)));

    /* the other way is to use an std::array rvalue */
    EXPECT_EQ(runTest("abc\ncde\n"sv, std::array<std::string_view, 2>{"abc", "cde"}), "");

    EXPECT_EQ(runTest("abc\r\ncde"sv, std::array<std::string_view, 2>{"abc", "cde"}), "");
    EXPECT_EQ(runTest("abc\r\ncde\r\n"sv, std::array<std::string_view, 2>{"abc", "cde"}), "");
    EXPECT_EQ(runTest("abc\r\ncde\r"sv, std::array<std::string_view, 2>{"abc", "cde"}), "");
    EXPECT_EQ(runTest("abc\rcde\r"sv, std::array<std::string_view, 1>{"abc\rcde"}), "");
    EXPECT_EQ(runTest("abc\ncde\r\n\n"sv, std::array<std::string_view, 3>{"abc", "cde", ""}), "");
    EXPECT_EQ(runTest("\r\r\n"sv, std::array<std::string_view, 1>{"\r"}), "");
    EXPECT_EQ(runTest("abc\n\r\n\ncde\r\r\n"sv, std::array<std::string_view, 4>{"abc", "", "", "cde\r"}), "");
}