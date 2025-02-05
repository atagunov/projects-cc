#pragma once

#include <iostream>
#include <fmt/core.h>
#include <fmt/ostream.h>

#include <boost/log/attributes.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/sources/severity_channel_logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/keywords/channel.hpp>

#include <boost/type_index.hpp>

/**
 * Adapter for using boost logging
 *
 * Presently all logging is going to the console
 * Logging can be directed to a file using regular Boost log facilities
 *
 * When passing arguments to std::format all values after format string go in as const& - seems good enough for now
 */
namespace util::log {
    // not very elegant that this creates util::log::src namespace but simplifes this file
    namespace src = boost::log::sources;

    // could use enum class
    enum severity_level {
        DEBUG, INFO, WARN, ERROR
    };

    std::ostream& operator<<(std::ostream& os, severity_level severity);

    void _appendException(boost::log::record_ostream&, const std::exception&);
    void _appendException(boost::log::record_ostream&, std::exception_ptr);

    namespace _detail {
        /** Concept checking if the last type in Args... has std::exception as base class */
        template <typename... Args>
        concept EndsInException = (sizeof...(Args) > 0) && std::is_base_of_v<std::exception,
                typename std::tuple_element<sizeof...(Args) - 1, std::tuple<Args...>>::type>;

        /** Class providing FormatStrintT<>, print<>() and getExc() */
        template <typename... Args> struct FormatHelper;

        /** Base case of template recursion */
        template <typename Exc> struct FormatHelper<Exc> {
            using ExcT = Exc;
            static const ExcT& getExc(const Exc& exc) {
                return exc;
            }

            template <typename... Prefixes> using FormatStringT = fmt::format_string<const Prefixes&...>;
            template <typename... Prefixes> static void print(boost::log::record_ostream& ros,
                    FormatStringT<Prefixes...> fmt, const Prefixes&... prefixes, const Exc&) {
                /* on c++23 we could have used fmt::print here, for now let's use {fmt} */
                fmt::print(ros.stream(), fmt, prefixes...);
            }
        };

        /** Recursive case */
        template <typename T, typename... Rest> struct FormatHelper<T, Rest...> {
            using ExcT = FormatHelper<Rest...>::ExcT;
            static const ExcT& getExc(const T& t, const Rest&... rest) {
                return FormatHelper<Rest...>::getExc(rest...);
            }

            /**
             * Builds format_string type out of any types from
             * - upper levels of recursion (Prefixes...)
             * - curren tlevel of recursion (T)
             * - lower levels of recursion (Rest...) - except trailing ExcT is excluded
             */
            template <typename... Prefixes> using FormatStringT = FormatHelper<Rest...>::template
                    FormatStringT<Prefixes..., T>;

            /**
             * Prints formatted message using parameters from
             * - upper level of recursion (prefixes...)
             * - this level of recursion (t)
             * - lower levels of recursion (rest...) - except trailing exception is not included
             */
            template <typename... Prefixes> static void print(boost::log::record_ostream& ros,
                    FormatStringT<Prefixes...> fmt, const Prefixes&... prefixes, const T& t, const Rest&... rest) {
                FormatHelper<Rest...>::template print<Prefixes..., T>(ros, fmt, prefixes..., t, rest...);
            }
        };

        /**
         * MARKER type here identifies a unique logger instance
         * and defines its name via Boost's pretty_name facility
         *
         * This method is not considered as a part of public API of util::log
         * rather the next method is
         */
        template<class T, typename MARKER> inline T& _getSingleton() {
            // since C++11 executed exactly once
            static T t{boost::typeindex::type_id<MARKER>().pretty_name()};
            return t;
        }

        /**
         * MARKER type here identifies a unique logger instance
         */
        template<class T, typename MARKER> inline T& _getThreadLocal() {
            // since C++11 executed exactly once
            thread_local T t{boost::typeindex::type_id<MARKER>().pretty_name()};
            return t;
        }
    }

    /**
     * This class is a template so that we can easily procude two versions of
     * thread-safe (MT) and non-thread-safe
     *
     * We possibly could have had a more elegant design if we used BOOST_LOG_DECLARE_LOGGER_TYPE macro
     */
    template<typename Parent>
    class _Logger: public Parent {
        using ros_t = boost::log::record_ostream;

        _Logger(std::string channel): Parent{boost::log::keywords::channel = channel} {}

        template<class T, typename MARKER>
        friend T& _detail::_getSingleton();

        template<class T, typename MARKER>
        friend T& _detail::_getThreadLocal();

        /** This version will get used if last arg is an exception */
        template<typename ...Args>
        void _logExc(severity_level severityLevel,
                _detail::FormatHelper<Args...>::template FormatStringT<> fmt,
                const Args&... args) {
            using boost::log::keywords::severity;

            if (auto record = this->open_record(severity = severityLevel)) {
                ros_t ros{record};
                _detail::FormatHelper<Args...>::template print<>(ros, fmt, args...);
                _appendException(ros, _detail::FormatHelper<Args...>::getExc(args...));
                this->push_record(std::move(record));
            }
        }

        /** This version will get used if last arg is not an exception of if there are no args */
        template<typename ...Args>
        void _log(severity_level severityLevel, fmt::format_string<const Args&...> fmt, const Args&... args) {
            using boost::log::keywords::severity;

            if (auto record = this->open_record(severity = severityLevel)) {
                ros_t ros{record};
                fmt::print(ros.stream(), fmt, args...);
                this->push_record(std::move(record));
            }
        }

        template<typename ...Args> void _logWithCurrentException(severity_level severityLevel,
                fmt::format_string<Args...> fmt, const Args&... args) {
            using boost::log::keywords::severity;

            if (auto record = this->open_record(severity = severityLevel)) {
                ros_t ros{record};
                fmt::print(ros.stream(), fmt, args...);
                _appendException(ros, std::current_exception());
                ros.flush();
                this->push_record(std::move(record));
            }
        }
    public:
        template<typename ...Args>
        requires _detail::EndsInException<Args...>
        void debug(_detail::FormatHelper<Args...>::template FormatStringT<> fmt, const Args&... args) {
            _logExc(DEBUG, fmt, args...);
        }

        template<typename ...Args>
        requires (!_detail::EndsInException<Args...>) // selects wrong template without this
        void debug(fmt::format_string<const Args&...> fmt, const Args&... args) {
            _log(DEBUG, fmt, args...);
        }

        template<typename ...Args>
        requires _detail::EndsInException<Args...>
        void info(_detail::FormatHelper<Args...>::template FormatStringT<> fmt, const Args&... args) {
            _logExc(INFO, fmt, args...);
        }

        template<typename ...Args>
        requires (!_detail::EndsInException<Args...>) // selects wrong template without this
        void info(fmt::format_string<const Args&...> fmt, const Args&... args) {
            _log(INFO, fmt, args...);
        }

        template<typename ...Args>
        requires _detail::EndsInException<Args...>
        void warn(_detail::FormatHelper<Args...>::template FormatStringT<> fmt, const Args&... args) {
            _logExc(WARN, fmt, args...);
        }

        template<typename ...Args>
        requires (!_detail::EndsInException<Args...>) // selects wrong template without this
        void warn(fmt::format_string<const Args&...> fmt, const Args&... args) {
            _log(WARN, fmt, args...);
        }

        template<typename ...Args>
        requires _detail::EndsInException<Args...>
        void error(_detail::FormatHelper<Args...>::template FormatStringT<> fmt, const Args&... args) {
            _logExc(ERROR, fmt, args...);
        }

        template<typename ...Args>
        requires (!_detail::EndsInException<Args...>) // selects wrong template without this
        void error(fmt::format_string<const Args&...> fmt, const Args&... args) {
            _log(ERROR, fmt, args...);
        }

        template<typename ...Args> void warnWithCurrentException(fmt::format_string<const Args&...> fmt, const Args&... args) {
            _logWithCurrentException(WARN, fmt, args...);
        }

        template<typename ...Args> void errorWithCurrentException(fmt::format_string<const Args&...> fmt, const Args&... args) {
            _logWithCurrentException(ERROR, fmt, args...);
        }

        /* Instances of _Logger can actually be copied but we want all usage to go via getLogger() */
        _Logger(const _Logger&) = delete;
        _Logger& operator= (const _Logger&) = delete;
    };

    /**
     * At present we've chosen to make the Logger a multi-threaded class
     * If we need non-mt class later it will then be called smth like FastLogger
     *
     * We could have done it the other way around and made Logger non-thread-safe
     * and then created separate LoggerMT with its separate getLoggerMt()
     */
    using Logger = _Logger<src::severity_channel_logger_mt<severity_level>>;

    /** Thread-local */
    using LoggerTL = _Logger<src::severity_channel_logger<severity_level>>;

    template <typename MARKER> inline auto getLogger() -> Logger& {
        return _detail::_getSingleton<Logger, MARKER>();
    }

    template <typename MARKER> inline auto getLoggerTL() -> LoggerTL& {
        return _detail::_getThreadLocal<LoggerTL, MARKER>();
    }

    /**
     * main() would call this function with levelsAbove = 1
     * that will mean that we shall take note of the address 1 level above the caller
     * and should we see it in a stack trace we shall stop printing the stacktrace
     * this helps us avoid printing addresses from libc
     */
    void suppressTracesAbove(std::size_t levelsAbove = 1);

    /**
     * Invokes boost::log::add_common_attributes() and adds timestamps and levels
     */
    void commonLoggingSetup();

    /**
     * Activate logging to console
     */
    void logToConsole();

    void setStandardLogFormat(boost::shared_ptr<boost::log::sinks::synchronous_sink<
            boost::log::sinks::text_ostream_backend>>);
}
