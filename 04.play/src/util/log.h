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
     *
     * Open question: could we have a more elegant design if we used BOOST_LOG_DECLARE_LOGGER_TYPE macro?
      */
    template<typename Parent>
    class _Logger: public Parent {
        using ros_t = boost::log::record_ostream;

        _Logger(std::string channel): Parent{boost::log::keywords::channel = channel} {}

        template<class Logger, typename MARKER>
        friend Logger& _getLogger();

        /** This 1st way to determine that last arg passed is an exception: __logExc */

        /** Helper function, invoked when last arg is of a class extending std::exception */
        template<typename ...Args, std::size_t ...Is>
        void _logExc(severity_level severityLevel, const std::tuple<Args...>& tup, std::index_sequence<Is...>) {
            using boost::log::keywords::severity;

            if (auto record = this->open_record(severity = severityLevel)) {
                ros_t ros{record};
                ((ros << std::get<Is>(tup)),...);
                _appendException(ros, std::get<sizeof...(Args) - 1>(tup));
                this->push_record(std::move(record));
            }
        }

        /** This version will get used if last arg is an exception */
        template<typename ...Args>
        requires (sizeof...(Args) > 0) && std::is_base_of_v<std::exception,
                typename std::tuple_element<sizeof...(Args) - 1, std::tuple<Args...>>::type>
        void _log(severity_level severityLevel, const Args&... args) {
            _logExc(severityLevel, std::tie(args...),
                    std::make_index_sequence<sizeof...(Args) - 1>{});
        }

        /** This version will get used if last arg is not an exception of if there are no args */
        template<typename ...Args>
        void _log(severity_level severityLevel, const Args&... args) {
            using boost::log::keywords::severity;

            if (auto record = this->open_record(severity = severityLevel)) {
                ros_t ros{record};
                (ros << ... << args);
                this->push_record(std::move(record));
            }
        }

        /* This is another way to determine that last arg passed is an exception - via a recursive template */
        /*template<typename A0, typename ...Args> void _logOneByOne(ros_t& ros,
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
        }*/

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
        }    struct handle_terminate_log{};
gs...);
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

        /* Instances of _Logger can actually be copied but we want all usage to go via getLogger() */
        _Logger(const _Logger&) = delete;
        _Logger& operator= (const _Logger&) = delete;
    };

    /**
     * MARKER type here identifies a unique logger instance
     * and defines its name via Boost's pretty_name facility
     *
     * This method is not considered as a part of public API of util::log
     * rather the next method is
     */
    template<class L, typename MARKER> inline L& _getLogger() {
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

    template <typename T> inline Logger& getLogger() {
        return _getLogger<Logger, T>();
    }

    void setupSimpleConsoleLogging();
}
