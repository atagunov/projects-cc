#pragma once

#include <iostream>

#include <boost/log/attributes.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/keywords/channel.hpp>

#include <boost/type_index.hpp>

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

    void _appendException(boost::log::record_ostream& ros, const std::exception& e);
    void _appendCurrentException(boost::log::record_ostream& ros);

    /**
     * This class is a template so that we can easily procude two versions of
     * thread-safe (MT) and non-thread-safe
     */
    template<typename Parent>
    class _Logger: public Parent {
        using ros_t = boost::log::record_ostream;

        _Logger(std::string channel): Parent{boost::log::keywords::channel = channel} {}

        template<class Logger, typename T>
        friend Logger _getLogger();

        template<typename A0, typename ...Args> void _logOneByOne(ros_t& ros,
                const A0& a0, const Args&... args) {
            ros << a0;
            _logOneByOne(ros, args...);
        }

        template<typename A0> void _logOneByOne(ros_t& ros, const A0& a0) {
            ros << a0;
        }

        template<typename E> requires std::is_base_of_v<std::exception, E>
        void _logOneByOne(ros_t& ros, const E& e) {
            _appendException(ros, e);
        }

        template<typename ...Args> void _log(severity_level severityLevel, const Args&... args) {
            using boost::log::keywords::severity;

            if (auto record = this->open_record(severity = severityLevel)) {
                ros_t ros{record};
                _logOneByOne(ros, args...);
                ros.flush();
                this->push_record(std::move(record));
            }
        }

        template<typename ...Args> void _logWithCurrentException(severity_level severityLevel, const Args&... args) {
            using boost::log::keywords::severity;

            if (auto record = this->open_record(severity = severityLevel)) {
                ros_t ros{record};
                (ros << ... << args);
                _appendCurrentException(ros);
                ros.flush();
                this->push_record(std::move(record));
            }
        }
    public:
        /** If the last argument passed extends std::exception we shall print stack trace all right */
        template<typename ...Args> void debug(const Args&... args) {
            _log(DEBUG, args...);
        }

        /** If the last argument passed extends std::exception we shall print stack trace all right */
        template<typename ...Args> void info(const Args&... args) {
            _log(INFO, args...);
        }

        /** If the last argument passed extends std::exception we shall print stack trace */
        template<typename ...Args> void warn(const Args&... args) {
            _log(WARN, args...);
        }

        /** If the last argument passed extends std::exception we shall print stack trace */
        template<typename ...Args> void error(const Args&... args) {
            _log(ERROR, args...);
        }

        template<typename ...Args> void warnWithCurrentException(const Args&... args) {
            _logWithCurrentException(WARN, args...);
        }

        template<typename ...Args> void errorWithCurrentException(const Args&... args) {
            _logWithCurrentException(ERROR, args...);
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
    using Logger = _Logger<src::severity_channel_logger_mt<severity_level>>;

    template <typename T> inline Logger getLogger() {
        return _getLogger<Logger, T>();
    }

    void setupSimpleConsoleLogging();
}
