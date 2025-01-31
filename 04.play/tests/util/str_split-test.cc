#include <util/str_split.h>
#include <gtest/gtest.h>
#include <string_view>
#include <iostream>

using namespace std::string_view_literals;

TEST(str_split, test) {
    util::str_split::StrSplitView view("abc\ncde"sv);
}