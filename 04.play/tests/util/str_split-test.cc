#include <util/str_split.h>
#include <gtest/gtest.h>
#include <string_view>
#include <iostream>
#include <span>
#include <algorithm>
#include <random>

#include <util/memory_counter.h>
#include <boost/config.hpp>

using util::str_split::LinesSplitView;
using namespace std::string_literals;
using namespace std::string_view_literals;

//  earlier version: template<typename EXPECTED> ... runTest(..., EXPECTED&& expected)
//  runTest(..., std::array<std::string_view, 2>{"abc", "cde"})

std::string runTest(const LinesSplitView<std::string_view> view,
        std::initializer_list<std::string_view> expected) {
    std::ostringstream s;

    if (!std::ranges::equal(view, expected)) {
        s << "Actual: ";
        std::ranges::copy(view, std::ostream_iterator<std::string_view>(s, ", "));
    }

    return (std::move(s)).str();
}

TEST(str_split, mainTest) {
    LinesSplitView view("abc\ncde"sv);
    std::string_view expectedA[] = {"abc", "cde"};
    EXPECT_TRUE(std::ranges::equal(view, std::span(expectedA)));

    EXPECT_EQ(runTest("abc\ncde\n"sv, {"abc", "cde"}), "");
    EXPECT_EQ(runTest("abc\r\ncde"sv,{"abc", "cde"}), "");
    EXPECT_EQ(runTest("abc\r\ncde\r\n"sv, {"abc", "cde"}), "");
    EXPECT_EQ(runTest("abc\r\ncde\r"sv, {"abc", "cde"}), "");
    EXPECT_EQ(runTest("abc\rcde\r"sv, {"abc\rcde"}), "");
    EXPECT_EQ(runTest("abc\ncde\r\n\n"sv, {"abc", "cde", ""}), "");
    EXPECT_EQ(runTest("\r\r\n"sv, {"\r"}), "");
    EXPECT_EQ(runTest("abc\n\r\n\ncde\r\r\n"sv, {"abc", "", "", "cde\r"}), "");
}

static constinit LinesSplitView staticView1("abc\ncde"sv);
TEST(str_split, constExprTest) {
    EXPECT_TRUE(std::ranges::equal(staticView1, std::array<std::string_view, 2>{"abc", "cde"}));
}

TEST(str_split, view) {
    std::string s{"a\nquite long really\nc"}; // std::string as an lvalue in case EXPECT_EQ not a single expression
    EXPECT_EQ(runTest(std::string_view{s}, {"a", "quite long really", "c"}), "");
}

TEST(str_split, sentinelWorks) {
    auto sv = "abcdef\nmore"sv;
    auto start = util::str_split::LinesSplitIterator{sv.begin(), sv.end()};
    EXPECT_FALSE(start == std::default_sentinel);
    EXPECT_FALSE(std::default_sentinel == start);
    EXPECT_TRUE(start != std::default_sentinel);
    EXPECT_TRUE(std::default_sentinel != start);
}

class TracedString: public std::string, util::memory_counter::MemoryCounter {
public:
    TracedString(const char* s, util::memory_counter::MemoryCounts& counts): std::string(s), MemoryCounter(counts) {}
    TracedString(const TracedString&) = default;
    TracedString(TracedString&&) = default;
    TracedString& operator=(const TracedString&) = default;
    TracedString& operator=(TracedString&&) = default;
};

/** Let's make sure we can use an owning view */
TEST(str_split, canUseOwningView) {
    util::memory_counter::MemoryCounts counts;
    {
        LinesSplitView view{TracedString{"abcdefghijklmopqrt\n14", counts}};
        EXPECT_TRUE(counts.check({.constructed = 1, .copied = 0, .freed = 0})) << " but it was " << counts;
    }
    EXPECT_TRUE(counts.check({.constructed = 1, .copied = 0, .freed = 1})) << " but it was " << counts;
}

TEST(str_split, canUseOwningViewExplicitly) {
    util::memory_counter::MemoryCounts counts;
    {
        LinesSplitView view{std::ranges::owning_view{TracedString{"abcdefghijklmopqrt\n14", counts}}};
        EXPECT_TRUE(counts.check({.constructed = 1, .copied = 0, .freed = 0})) << " but it was " << counts;
    }
    EXPECT_TRUE(counts.check({.constructed = 1, .copied = 0, .freed = 1})) << " but it was " << counts;
}

TEST(str_split, canUseRefView) {
    util::memory_counter::MemoryCounts counts;
    {
        /* std::string functions as a range not view */
        auto range = TracedString{"abcdefghijklmopqrt\n14", counts};
        /* this still works because std::ranges::views::all_t conversion is happening constructing RefView temporary */
        LinesSplitView view{range};
        EXPECT_TRUE(counts.check({.constructed = 1, .copied = 0, .freed = 0})) << " but it was " << counts;
    }
    EXPECT_TRUE(counts.check({.constructed = 1, .copied = 0, .freed = 1})) << " but it was " << counts;
}

class TracedStringView: public std::ranges::view_interface<TracedStringView>, public std::string_view, util::memory_counter::MemoryCounter {
public:
    TracedStringView(const std::string& s, util::memory_counter::MemoryCounts& counts): std::string_view(s), MemoryCounter(counts) {}
    TracedStringView(const TracedStringView&) = default;
    TracedStringView(TracedStringView&&) = default;
    TracedStringView& operator=(const TracedStringView&) = default;
    TracedStringView& operator=(TracedStringView&&) = default;
};

static_assert(std::ranges::view<TracedStringView>);

TEST(str_split, acceptsViewRValue) {
    util::memory_counter::MemoryCounts counts;
    {
        LinesSplitView view{TracedStringView{"abcdefghijklmopqrt\n14", counts}};
        EXPECT_TRUE(counts.check({.constructed = 1, .copied = 0, .freed = 0})) << " but it was " << counts;
    }
    EXPECT_TRUE(counts.check({.constructed = 1, .copied = 0, .freed = 1})) << " but it was " << counts;
}

TEST(str_split, acceptsViewLValue) {
    util::memory_counter::MemoryCounts counts;
    TracedStringView underlyingView{"abcdefghijklmopqrt\n14", counts};
    {
        LinesSplitView view{underlyingView};
        EXPECT_TRUE(counts.check({.constructed = 1, .copied = 1, .freed = 0})) << " but it was " << counts;
    }
    EXPECT_TRUE(counts.check({.constructed = 1, .copied = 1, .freed = 1})) << " but it was " << counts;
}

TEST(str_split, acceptsViewLValueViaStdMove) {
    util::memory_counter::MemoryCounts counts;
    TracedStringView underlyingView{"abcdefghijklmopqrt\n14", counts};
    {
        LinesSplitView<TracedStringView> view{std::move(underlyingView)};
        EXPECT_TRUE(counts.extendedCheck({.constructed = 1, .copied = 0, .freed = 0, .moved = 2})) << " but it was " << counts;
    }
    EXPECT_TRUE(counts.extendedCheck({.constructed = 1, .copied = 0, .freed = 1, .moved = 2})) << " but it was " << counts;
}
