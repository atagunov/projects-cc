#pragma once

#include <iostream>

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
    // not very elegant that this creates util::log::src namespace but simplifes this file
    namespace src = boost::log::sources;

    // could use enum class
    enum severity_level {
        DEBUG, INFO, WARN, ERROR
    };

    std::ostream& operator<<(std::ostream& os, severity_level severity);
    void _appendCurrentException(boost::log::record_ostream& ros);

    /**
     * This class is a template so that we can easily procude two versions of
     * thread-safe (MT) and non-thread-safe
     */
    template<typename Threading>
    class _Logger: public src::basic_composite_logger<char,
            _Logger<Threading>,
            Threading, 
            src::features<src::severity<severity_level>>> {
        using record_ostream = boost::log::record_ostream;

        _Logger(std::string channel) {
            // doesn't resolve add_attribute from parent w/o this->
            this->add_attribute("Channel", boost::log::attributes::constant<std::string>{channel});
        }

        template<class Logger, typename T>
        friend Logger _getLogger();

        template<typename ...Args> void doLog(severity_level severityLevel, const Args&... args) {
            using boost::log::keywords::severity;

            if (auto record = this->open_record(severity = severityLevel)) {
                record_ostream ros{record};
                (ros << ... << args);
                _appendCurrentException(ros);
                ros.flush();
                this->push_record(std::move(record));
            }
        }
    public:
        /**
         * Prints error message and info on current exception including stack trace
         */
        template<typename ...Args> void error(const Args&... args) {
            doLog(ERROR, args...);
        }

        template<typename ...Args> void info(const Args&... args) {
            doLog(INFO, args...);
        }
    };

    /**
     * MARKER type here identifies a unique logger instance
     * and defines its name via Boost's pretty_name facility
     *
     * This method is not considered as a part of public API of util::log
     * rather the next method is
     */
    template<class L, typename MARKER> inline L _getLogger() {
        // since C++11 executed exactly once
        static L logger{boost::typeindex::type_id<MARKER>().pretty_name()};
        return logger;
    }

    /**
     * At present we've chosen to make the Logger a multi-threaded class
     * If we need non-mt class later it will then be called smth like FastLogger
     *
     * We could have done it the other way around and made Logger non-thread-safe
     * and then created separate LoggerMT with its separate getLoggerMt()
     */
    using Logger = _Logger<src::multi_thread_model<boost::log::aux::light_rw_mutex>>;

    template <typename T> inline Logger getLogger() {
        return _getLogger<Logger, T>();
    }

    void setupSimpleConsoleFormat();
}
