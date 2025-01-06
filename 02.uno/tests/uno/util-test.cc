#include <uno/util.cc>
#include <gtest/gtest.h>

namespace uno::util {
    static int ensureOnce;

    TEST(util, sameFunctionWorks) {
        EXPECT_EQ(someFunction(1), 2);
    }

    TEST(util, ensureEachTestCaseForkedA) {
        EXPECT_EQ(ensureOnce++, 0);
    }

    /**
     * Here we're trying to ensure each of our test cases is run in a separate process
     *
     * This is achieved with "ctest" so long as gtest_discover_tests is used
     * and in vs code via CMake Tool extension
     *
     * This is presently not achieved with TestMate in vs code
     */
    TEST(util, ensureEachTestCaseForkedB) {
        EXPECT_EQ(ensureOnce++, 0);
    }
}
