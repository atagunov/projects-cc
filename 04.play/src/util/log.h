#pragma once

#include <tuple>

#include <boost/log/core.hpp>
#include <boost/log/attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>

#include <boost/type_index.hpp>

#include <boost/stacktrace.hpp>

/**
 * Adapter for using boost logging
 *
 * TODO: stack traces printed do not show where currently executing frame is
 * making it harder to read the log
 *
 * TODO: shift stack trace items further to the right to make stack
 * traces easier to read
 *
 * Presently all logging is going to the console
 * Logging can be directed to a file using regular Boost log facilities
 */
namespace util::log {
    // could use enum class
    enum severity_level {
        DEBUG, INFO, WARN, ERROR
    };

    class Logger: public boost::log::sources::severity_logger_mt<severity_level> {
        using record_ostream = boost::log::record_ostream;

        Logger(std::string channel) {
            add_attribute("Channel", boost::log::attributes::constant<std::string>{channel});
        }

        template<typename T>
        friend Logger getLogger();

        void appendCurrentException(record_ostream& ros);

    public:
        /**
         * Prints error message and info on current exception including stack trace
         */
        template<typename ...Args>
        void error(const Args&... args) {
            using boost::log::keywords::severity;
            using boost::stacktrace::stacktrace;

            if (auto record = open_record(severity = ERROR)) {
                record_ostream ros{record};
                (ros << ... << args);
                appendCurrentException(ros);
                ros.flush();
                push_record(std::move(record));
            }
        }
    };

    template<typename T> inline
    Logger getLogger() {
        // since C++11 executed exactly once
        static Logger logger{boost::typeindex::type_id<T>().pretty_name()};
        return logger;
    }
}
