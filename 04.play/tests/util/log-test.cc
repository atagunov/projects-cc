#include <util/log.h>
#include <util/str_split.h>
#include <gtest/gtest.h>

#include <iostream>
#include <print>

#include <boost/log/sinks.hpp>

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

TEST_F(LogTests, infoWorks) {
    auto& logger = util::log::getLogger<test>();
    logger.info("This is a test message with an int {} and a float {}", 42, 42.0f);
    std::string result{extractResult()};
    //EXPECT_EQ("foo", extractResult());
}

TEST_F(LogTests, errorWorks) {

}




