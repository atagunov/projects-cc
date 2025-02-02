#include <util/log.h>
#include <util/str_split.h>
#include <gtest/gtest.h>

#include <iostream>
#include <print>
#include <exception>
#include <future>

#include <boost/log/sinks.hpp>
#include <boost/config.hpp>

namespace sinks=boost::log::sinks;
namespace logging=boost::log;
using text_sink=sinks::synchronous_sink<sinks::text_ostream_backend>;

class LogTests : public testing::Test {
protected:
    static void SetUpTestSuite() {
        sink->locked_backend()->add_stream(logOutput);
        util::log::setStandardLogFormat(sink);

        logging::core::get()->add_sink(sink);
        util::log::commonLoggingSetup();
    }

    static std::string extractResult() {
        sink->flush();
        std::stringbuf buf;
        swap(*(logOutput->rdbuf()), buf);
        return std::move(buf).str();
    }

private:
    inline static boost::shared_ptr<std::ostringstream> logOutput{
        boost::make_shared<std::ostringstream>()};
    inline static boost::shared_ptr<text_sink> sink{
        boost::make_shared<text_sink>()};
};

struct test{};

using namespace std::string_view_literals;

auto splitToVec(const std::string& str) {
    return std::ranges::to<std::vector>(util::str_split::LinesSplitView{
            std::string_view{str}});
}

TEST_F(LogTests, basicLoggingWorks) {
    auto& logger = util::log::getLogger<test>();
    logger.info("This is a test message with an int {} and a float {}", 42, 42.0f);
    logger.error("This is an error, some info: {}", std::array<int, 2>{17, 45});
    logger.debug("here's some debug", std::logic_error("test"));
    logger.warn("Here's a warning: {} != {}", std::pair(2, 4), 1'000'000'000'000'000'000L);

    std::string result{extractResult()}; // got to put into a local so that it doesn't get released before we're done with it
    auto lines = splitToVec(result);

    ASSERT_EQ(4, lines.size());

    /* using std::string_view literals as they're more efficient for ends_with; doesn't matter for a test of course */
    EXPECT_TRUE(lines[0].ends_with(" #INFO  [test] This is a test message with an int 42 and a float 42"sv))
            << "but it is " << lines[0];
    EXPECT_TRUE(lines[1].ends_with(" #ERROR [test] This is an error, some info: [17, 45]"sv))
            << "but it is " << lines[1];
    EXPECT_TRUE(lines[2].ends_with(" #DEBUG [test] here's some debug: std::logic_error(test)"sv))
            << "but it is " << lines[2];
    EXPECT_TRUE(lines[3].ends_with(" #WARN  [test] Here's a warning: (2, 4) != 1000000000000000000"sv))
            << "but it is " << lines[3];
}

namespace testexc {
    /** Using a namespace to see "some::TestException" in the log */
    class TestException: public std::logic_error {
        using logic_error::logic_error;
    };
}

BOOST_NOINLINE
void a_a() {
    throw testexc::TestException("Some interesting message");
}

BOOST_NOINLINE
void a_b() {
    a_a();
}

BOOST_NOINLINE
void a_c() {
    a_b();
}

void doTestSimpleException(std::string result) {
    auto lines = splitToVec(result);

    ASSERT_TRUE(lines.size() > 3);
    EXPECT_TRUE(lines[0].ends_with(" #ERROR [test] Oh! It's an exception: testexc::TestException(Some interesting message)"sv))
            << " but it is " << lines[0];

    /* using simple char* literals, they're ok for starts_with */
    EXPECT_TRUE(lines[1].starts_with("\t@ a_a()") || lines[1].starts_with("\t@ 0x"))
            << " but it is " << lines[1];
    EXPECT_TRUE(lines[2].starts_with("\t@ a_b()") || lines[2].starts_with("\t@ 0x"))
            << " but it is " << lines[2];
    EXPECT_TRUE(lines[3].starts_with("\t@ a_c()") || lines[3].starts_with("\t@ 0x"))
            << " but it is " << lines[3];
}

TEST_F(LogTests, simple) {
    auto& logger = util::log::getLogger<test>();
    try {
        a_c();
    } catch (std::exception& e) {
        logger.error("Oh! It's an exception", e);
    }

    doTestSimpleException(extractResult());
}

TEST_F(LogTests, simpleWithCurrent) {
    auto& logger = util::log::getLogger<test>();
    try {
        a_c();
    } catch (...) {
        logger.errorWithCurrentException("Oh! It's an exception");
    }

    doTestSimpleException(extractResult());
}

BOOST_NOINLINE
void b_a() {
    throw testexc::TestException("Root Exception");
}

BOOST_NOINLINE
void b_b() {
    b_a();
}

BOOST_NOINLINE
void b_c() {
    try {
        b_b();
    } catch (...) {
        std::throw_with_nested(testexc::TestException("Wrapping Exception"));
    }
}

BOOST_NOINLINE
void b_d() {
    b_c();
}

BOOST_NOINLINE
void b_e() {
    b_d();
}

void doTestNestedException(std::string result) {
    auto lines = splitToVec(result);

    ASSERT_TRUE(lines.size() > 3);
    EXPECT_TRUE(lines[0].contains(" #ERROR [test] Nested test") && lines[0].contains("testexc::TestException")
            && lines[0].contains("(Wrapping Exception)")) << " but it is " << lines[0];
    EXPECT_TRUE(std::ranges::any_of(lines, [](const auto& line){
            return line.starts_with("\tcaused by: testexc::TestException(Root Exception)"); }))
            << " but it is " << result;
}

TEST_F(LogTests, nested) {
    auto& logger = util::log::getLogger<test>();
    try {
        b_d();
    } catch (std::exception& e) {
        logger.error("Nested test", e);
    }

    doTestNestedException(extractResult());
}

TEST_F(LogTests, nestedWithCurrent) {
    auto& logger = util::log::getLogger<test>();
    try {
        b_d();
    } catch (std::exception& e) {
        logger.errorWithCurrentException("Nested test");
    }

    doTestNestedException(extractResult());
}

TEST_F(LogTests, fromAnotherThread) {
    auto& logger = util::log::getLogger<test>();
    auto future = std::async(std::launch::any, a_c);
    try {
        future.get();
    } catch (std::exception& e) {
        logger.error("Oh! It's an exception", e);
    }

    doTestSimpleException(extractResult());
}
