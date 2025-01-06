#include <uno/util.cc>
#include <gtest/gtest.h>

namespace uno::util {
    TEST(util, sameFunctionWorks) {
        EXPECT_EQ(someFunction(1), 2);
    }
}
