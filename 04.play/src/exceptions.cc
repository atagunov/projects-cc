#include <iostream>
#include <exception>
#include <chrono>
#include <future>

#include <util/log.h>

using std::string;
using std::logic_error;

using util::log::Logger;
using util::log::LoggerTL;

using util::log::getLogger;
using util::log::getLoggerTL;

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

void sub_routine() {
    auto& logger = getLogger<struct main>();

    try {
        f1();
    } catch (std::exception& e) {
        logger.error("Error while doing - here's a float {} and a boolean {} to test formatting - f1()", 3.14f, false, e);
    }
}

void dealWithExcOnThatThread(std::exception_ptr ptr) {
    try {
        std::rethrow_exception(ptr);
    } catch (...) {
        auto& logger = getLogger<struct main>();
        logger.errorWithCurrentException("Now caught exception from a diff thread");
    }
}

int main() {
    util::log::suppressTracesAbove();
    util::log::commonLoggingSetup();
    util::log::logToConsole();

    auto& logger = getLogger<struct main>();
    logger.info("Starting; here's an int/float/long to check formatting: {}/{}/{}", 18, 1.0f, 1000l);

    sub_routine();

    /*logger.warn("Round 2");
    try {
        f1();
    } catch (...) {
        logger.errorWithCurrentException("Something went wrong again doing f1()");
    }*/

    auto future = std::async(std::launch::async, f1);
    try {
        future.get();
    } catch (...) {
        logger.errorWithCurrentException("exception from future");
    }

    auto future2 = std::async(std::launch::async, f1);
    std::optional<std::exception_ptr> exc;
    try {
        future2.get();
    } catch (...) {
        logger.info("wrapping exception...");
        exc = std::current_exception();
    }

    if (exc.has_value() && exc.value()) {
        std::async(std::launch::async, dealWithExcOnThatThread, exc.value()).get();
    } else {
        logger.error("No exception to pay with");
    }

    throw some::TestException("from-main");

    logger.info("Exiting");
    return 0;
}
