#include <iostream>
#include <exception>
#include <chrono>

#include <util/log.h>

using std::string;
using std::logic_error;

using util::log::Logger;
using util::log::getLogger;

using util::log::INFO;
using util::log::ERROR;

namespace some {
    /** Using a namespace to see "some::TestException" in the log */
    class TestException: public logic_error {
        using logic_error::logic_error;
    };
}

namespace {
    void f5() {
        throw some::TestException("test");
    }

    void f4() {
        f5();
    }

    void f3() {
        try {
            f4();
        } catch (std::exception& e) {
            std::throw_with_nested(std::logic_error{"smth wrong in f3"});
        }
    }

    void f2() {
        f3();
    }

    void f1() {
        f2();
    }
}

// marker struct used to identify a globally unique "main" logger
// any part of the app can call getLogger<main> and get the same logger instance
struct main{};

int main() {
    util::log::setupSimpleConsoleLogging();

    Logger logger = getLogger<struct main>();
    logger.info("Staring");

    try {
        f1();
    } catch (std::exception& e) {
        // will print current exception too
        logger.error("Error while doing f1");
    }


    logger.info("Exiting");
    return 0;
}
